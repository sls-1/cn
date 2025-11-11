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
int sockfd = -1;

void *recv_thread(void *arg) {
    char buf[BUF];
    while (!done) {
        ssize_t n = recv(sockfd, buf, BUF-1, 0);
        if (n <= 0) { done = 1; break; }
        buf[n] = '\0';
        printf("Remote: %s", buf);
        fflush(stdout);
        if (strncmp(buf, "exit", 4) == 0) { done = 1; break; }
    }
    return NULL;
}

void *send_thread(void *arg) {
    char buf[BUF];
    while (!done && fgets(buf, BUF, stdin) != NULL) {
        ssize_t n = send(sockfd, buf, strlen(buf), 0);
        if (n <= 0) { done = 1; break; }
        if (strncmp(buf, "exit", 4) == 0) { done = 1; break; }
    }
    return NULL;
}

int main(void) {
    struct sockaddr_in serv;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr) <= 0) { perror("inet_pton"); close(sockfd); exit(1); }

    if (connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("connect"); close(sockfd); exit(1); }
    puts("Connected to server");

    pthread_t rtid, stid;
    pthread_create(&rtid, NULL, recv_thread, NULL);
    pthread_create(&stid, NULL, send_thread, NULL);

    pthread_join(stid, NULL);
    pthread_join(rtid, NULL);

    close(sockfd);
    return 0;
}
