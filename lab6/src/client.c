#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <inttypes.h>  // для PRIu64

#include "excr_3.h"

struct Server {
    char ip[255];
    int port;
};

struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

void *ClientThread(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;

    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed for %s\n", data->server.ip);
        return NULL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(data->server.port);
    // Копируем адрес из h_addr_list[0]
    memcpy(&server.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        return NULL;
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        return NULL;
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        return NULL;
    }

    memcpy(&data->result, response, sizeof(uint64_t));
    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};
    int k_set = 0, mod_set = 0; // флаги, что аргументы заданы

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}};
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        if (ConvertStringToUI64(optarg, &k) != 0) {
                            fprintf(stderr, "Invalid k\n");
                            return 1;
                        }
                        k_set = 1;
                        break;
                    case 1:
                        if (ConvertStringToUI64(optarg, &mod) != 0) {
                            fprintf(stderr, "Invalid mod\n");
                            return 1;
                        }
                        mod_set = 1;
                        break;
                    case 2:
                        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                        break;
                    default: printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?': printf("Arguments error\n"); break;
            default: fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (!k_set || !mod_set || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Чтение файла с серверами
    FILE *f = fopen(servers_file, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    struct Server *servers = NULL;
    int servers_num = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        if (strlen(line) == 0) continue;

        char *colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid line format (expected ip:port): %s\n", line);
            continue;
        }
        *colon = '\0';
        servers = realloc(servers, sizeof(struct Server) * (servers_num + 1));
        strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
        servers[servers_num].port = atoi(colon + 1);
        servers_num++;
    }
    fclose(f);

    if (servers_num == 0) {
        fprintf(stderr, "No servers found\n");
        return 1;
    }

    pthread_t threads[servers_num];
    struct ThreadData data[servers_num];
    uint64_t total_range = k; // от 1 до k включительно (k чисел)
    uint64_t base = total_range / servers_num;
    uint64_t remainder = total_range % servers_num;
    uint64_t current = 1;

    for (int i = 0; i < servers_num; i++) {
        uint64_t part = base + (i < remainder ? 1 : 0);
        data[i].server = servers[i];
        data[i].begin = current;
        data[i].end = current + part - 1;
        data[i].mod = mod;
        current += part;

        if (pthread_create(&threads[i], NULL, ClientThread, &data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    uint64_t total = 1;
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        total = MultModulo(total, data[i].result, mod);
    }

    printf("answer: %" PRIu64 "\n", total);
    free(servers);
    return 0;
}