#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
    pthread_mutex_lock(&mutex1);
    printf("Поток 1: захватил mutex1\n");
    sleep(1); // даём время второму потоку захватить mutex2
    printf("Поток 1: пытается захватить mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 1: захватил mutex2, работа завершена\n");
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void *thread2(void *arg) {
    pthread_mutex_lock(&mutex2);
    printf("Поток 2: захватил mutex2\n");
    sleep(1);
    printf("Поток 2: пытается захватить mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 2: захватил mutex1, работа завершена\n");
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}