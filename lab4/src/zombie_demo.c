#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) завершается и становится зомби...\n", getpid());
        exit(0);
    } else {
        // Родительский процесс
        printf("Родительский процесс (PID: %d)\n", getpid());
        printf("Дочерний процесс (PID: %d) сейчас будет в статусе зомби 15 секунд.\n", pid);
        sleep(15);

        printf("Теперь родитель вызывает wait() для очистки зомби.\n");
        wait(NULL);

        printf("Зомби очищен. Программа завершается.\n");
    }
    return 0;
}