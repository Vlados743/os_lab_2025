#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>
#include <signal.h>
#include <time.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;                      // таймаут в секундах (0 - нет)
  bool with_files = false;

  // Разбор аргументов командной строки
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // проверка seed
            if (seed <= 0) {
              fprintf(stderr, "Ошибка: seed должен быть положительным числом\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              fprintf(stderr, "Ошибка: размер массива должен быть положительным\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              fprintf(stderr, "Ошибка: количество процессов должно быть положительным\n");
              return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout < 0) {
              fprintf(stderr, "Ошибка: таймаут не может быть отрицательным\n");
              return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
            printf("Индекс %d вне допустимого диапазона\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt вернул код 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Обнаружены лишние аргументы\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Использование: %s --seed число --array_size число --pnum число [--timeout число] [--by_files]\n",
           argv[0]);
    return 1;
  }

  // Генерируем массив
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Массив для хранения PID дочерних процессов (нужен для таймаута)
  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);

  // Если используем pipe, создаём по одному каналу на каждого ребёнка
  int (*pipes)[2] = NULL;  // указатель на массив из двух int
  if (!with_files) {
    pipes = malloc(sizeof(int) * pnum * 2);  // pnum каналов, каждый по 2 дескриптора
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int active_child_processes = 0;

  // Создаём процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes++;
      if (child_pid == 0) {
        // ---- Дочерний процесс ----
        // Определяем границы части массива
        int chunk_size = array_size / pnum;
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        // Вычисляем min и max в своей части
        struct MinMax part = GetMinMax(array, begin, end);

        if (with_files) {
          // Вариант с файлами: пишем результат в файл
          char filename[32];
          snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f) {
            fprintf(f, "%d %d\n", part.min, part.max);
            fclose(f);
          } else {
            perror("fopen");
          }
        } else {
          // Вариант с pipe: закрываем ненужный конец для чтения, пишем в pipe
          close(pipes[i][0]);               // закрываем чтение
          write(pipes[i][1], &part, sizeof(struct MinMax));
          close(pipes[i][1]);               // закрываем запись после отправки
        }
        exit(0);  // завершаем дочерний процесс
      } else {
        // Родитель: сохраняем PID ребёнка
        child_pids[i] = child_pid;
        // Если используем pipe, родитель закрывает свой конец записи (будет только читать)
        if (!with_files) {
          close(pipes[i][1]);  // закрываем запись в родителе
        }
      }
    } else {
      printf("Ошибка fork\n");
      return 1;
    }
  }

  // ---- Родительский процесс ----
  // Ожидаем завершения детей с учётом таймаута
  int remaining = active_child_processes;
  time_t wait_start = time(NULL);

  while (remaining > 0) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);  // неблокирующая проверка

    if (pid > 0) {
      // Какой-то ребёнок завершился
      remaining--;
    } else if (pid == 0) {
      // Нет завершившихся детей – проверяем таймаут
      if (timeout > 0 && (time(NULL) - wait_start) >= timeout) {
        // Таймаут истёк – убиваем всех оставшихся детей
        for (int i = 0; i < pnum; i++) {
          if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
          }
        }
        // Дожидаемся их окончательного завершения (чтобы не было зомби)
        while ((pid = wait(NULL)) > 0) {
          remaining--;
        }
        break;  // выходим из цикла
      }
    } else {
      // Ошибка waitpid
      perror("waitpid");
      break;
    }
  }

  // Собираем результаты от детей
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    struct MinMax part;
    part.min = INT_MAX;
    part.max = INT_MIN;

    if (with_files) {
      // Читаем из файла
      char filename[32];
      snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        if (fscanf(f, "%d %d", &part.min, &part.max) != 2) {
          fprintf(stderr, "Ошибка чтения файла %s\n", filename);
        }
        fclose(f);
        remove(filename);  // удаляем временный файл
      } else {
        // Если файл не открылся (например, ребёнок был убит по таймауту) – пропускаем
        continue;
      }
    } else {
      // Читаем из pipe
      int n = read(pipes[i][0], &part, sizeof(struct MinMax));
      if (n != sizeof(struct MinMax)) {
        // Если не удалось прочитать (ребёнок убит) – пропускаем
        continue;
      }
      close(pipes[i][0]);  // закрываем дескриптор чтения
    }

    if (part.min < min_max.min) min_max.min = part.min;
    if (part.max > min_max.max) min_max.max = part.max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);
  if (!with_files) {
    free(pipes);
  }

  printf("Минимум: %d\n", min_max.min);
  printf("Максимум: %d\n", min_max.max);
  printf("Затраченное время: %f мс\n", elapsed_time);
  fflush(NULL);
  return 0;
}