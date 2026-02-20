#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/time.h>
#include "utils.h"
#include "sum.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  uint64_t *result = malloc(sizeof(uint64_t));
  *result = Sum(sum_args);
  return result;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (1) {
    static struct option options[] = {
      {"threads_num", required_argument, 0, 0},
      {"seed", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            threads_num = atoi(optarg);
            if (threads_num <= 0) {
              fprintf(stderr, "Ошибка: threads_num > 0\n");
              return 1;
            }
            break;
          case 1:
            seed = atoi(optarg);
            if (seed <= 0) {
              fprintf(stderr, "Ошибка: seed > 0\n");
              return 1;
            }
            break;
          case 2:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              fprintf(stderr, "Ошибка: array_size > 0\n");
              return 1;
            }
            break;
        }
        break;
      default:
        fprintf(stderr, "Неизвестный аргумент\n");
        return 1;
    }
  }

  if (threads_num == 0 || array_size == 0 || seed == 0) {
    fprintf(stderr, "Использование: %s --threads_num N --seed S --array_size M\n", argv[0]);
    return 1;
  }

  pthread_t threads[threads_num];

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  struct SumArgs args[threads_num];

  int base_chunk = array_size / threads_num;
  int remainder = array_size % threads_num;
  int current_pos = 0;

  for (uint32_t i = 0; i < threads_num; i++) {
    int chunk = base_chunk + (i < remainder ? 1 : 0);
    args[i].array = array;
    args[i].begin = current_pos;
    args[i].end = current_pos + chunk;
    current_pos += chunk;
  }

  struct timeval start, end;
  gettimeofday(&start, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  uint64_t total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *result;
    pthread_join(threads[i], &result);
    total_sum += *(uint64_t *)result;
    free(result);
  }

  gettimeofday(&end, NULL);
  double elapsed = (end.tv_sec - start.tv_sec) * 1000.0;
  elapsed += (end.tv_usec - start.tv_usec) / 1000.0;

  free(array);
  printf("Total: %lu\n", total_sum);
  printf("Elapsed time: %f ms\n", elapsed);
  return 0;
}