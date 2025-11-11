#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
#define BUF 1024

volatile sig_atomic_t done = 0;
int sockfd;

void *recv_thread(void *arg) {
    char buf[BUF];
    while (!done) {
        ssize_t n = recv(sockfd, buf, BUF - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }
    done = 1;
    return NULL;
}

void *send_thread(void *arg) {
    char buf[BUF];
    while (!done && fgets(buf, BUF, stdin)) {
        send(sockfd, buf, strlen(buf), 0);
        if (strncmp(buf, "exit", 4) == 0) { done = 1; break; }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server. Type messages, 'exit' to quit.\n");

    pthread_t t1, t2;
    pthread_create(&t1, NULL, recv_thread, NULL);
    pthread_create(&t2, NULL, send_thread, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    close(sockfd);
    return 0;
}
