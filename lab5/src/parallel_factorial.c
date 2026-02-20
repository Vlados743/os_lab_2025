#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <stdint.h>

// Структура для передачи данных в поток
typedef struct {
    uint64_t start;      // начало диапазона
    uint64_t end;        // конец диапазона
    uint64_t mod;        // модуль
    uint64_t result;     // результат потока (произведение чисел своего диапазона по модулю)
} ThreadData;

// Общая переменная для итогового результата и мьютекс для её защиты
uint64_t total = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *ThreadFactorial(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    uint64_t local = 1;

    // Считаем произведение чисел от start до end по модулю mod
    for (uint64_t i = data->start; i <= data->end; i++) {
        local = (local * i) % data->mod;
    }
    data->result = local;

    // Захватываем мьютекс и умножаем общий результат на результат потока
    pthread_mutex_lock(&mutex);
    total = (total * data->result) % data->mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0, mod = 0;
    int pnum = 0;

    // Разбор аргументов командной строки
    while (1) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: k = atoll(optarg); break;
                    case 1: pnum = atoi(optarg); break;
                    case 2: mod = atoll(optarg); break;
                }
                break;
            default:
                fprintf(stderr, "Неизвестный аргумент\n");
                return 1;
        }
    }

    // Проверка, что все аргументы заданы
    if (k == 0 || mod == 0 || pnum == 0) {
        fprintf(stderr, "Использование: %s --k число --pnum N --mod M\n", argv[0]);
        return 1;
    }

    pthread_t threads[pnum];
    ThreadData data[pnum];

    // Разделяем диапазон чисел от 2 до k на pnum частей
    uint64_t range = k - 1;           // количество чисел от 2 до k
    uint64_t base = range / pnum;      // базовый размер части
    uint64_t rem = range % pnum;       // остаток, который распределим по первым потокам
    uint64_t current = 2;              // начинаем с 2 (факториал 1! = 1, его можно не считать)

    for (int i = 0; i < pnum; i++) {
        uint64_t chunk = base + (i < rem ? 1 : 0);  // размер части для этого потока
        data[i].start = current;
        data[i].end = current + chunk - 1;
        data[i].mod = mod;
        current += chunk;

        pthread_create(&threads[i], NULL, ThreadFactorial, &data[i]);
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%llu! mod %llu = %llu\n", k, mod, total);
    return 0;
}