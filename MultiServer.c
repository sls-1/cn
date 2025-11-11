#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
#define BUF 1024
#define MAX_CLIENTS 50

int clients[MAX_CLIENTS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t running = 1;

void broadcast(const char *msg, int from_fd) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != from_fd)
            send(clients[i], msg, strlen(msg), 0);
    }
    pthread_mutex_unlock(&lock);
}

void add_client(int fd) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] == -1) { clients[i] = fd; break; }
    pthread_mutex_unlock(&lock);
}

void remove_client(int fd) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] == fd) { clients[i] = -1; break; }
    pthread_mutex_unlock(&lock);
}

void *client_handler(void *arg) {
    int fd = *(int*)arg;
    free(arg);
    char buf[BUF];

    while (1) {
        ssize_t n = recv(fd, buf, BUF - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("Client %d: %s", fd, buf);
        if (strncmp(buf, "exit", 4) == 0) break;
        broadcast(buf, fd);
    }
    printf("Client %d disconnected\n", fd);
    remove_client(fd);
    close(fd);
    return NULL;
}

void *stdin_thread(void *arg) {
    char buf[BUF];
    while (fgets(buf, BUF, stdin)) {
        if (strncmp(buf, "exit", 4) == 0) {
            running = 0;
            break;
        }
        broadcast(buf, -1);
    }
    return NULL;
}

int main() {
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(srv, 10) < 0) { perror("listen"); exit(1); }

    printf("Server listening on %d\n", PORT);

    pthread_t stid;
    pthread_create(&stid, NULL, stdin_thread, NULL);

    while (running) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int *cfd = malloc(sizeof(int));
        *cfd = accept(srv, (struct sockaddr*)&cli, &len);
        if (*cfd < 0) { free(cfd); continue; }

        printf("Client %d connected\n", *cfd);
        add_client(*cfd);

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, cfd);
        pthread_detach(tid);
    }

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] != -1) close(clients[i]);
    pthread_mutex_unlock(&lock);

    close(srv);
    printf("Server shutting down\n");
    return 0;
}
