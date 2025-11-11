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
int connfd = -1;

void *recv_thread(void *arg) {
    char buf[BUF];
    while (!done) {
        ssize_t n = recv(connfd, buf, BUF-1, 0);
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
        ssize_t n = send(connfd, buf, strlen(buf), 0);
        if (n <= 0) { done = 1; break; }
        if (strncmp(buf, "exit", 4) == 0) { done = 1; break; }
    }
    return NULL;
}

int main(void) {
    int srv;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(srv); exit(1); }
    if (listen(srv, 1) < 0) { perror("listen"); close(srv); exit(1); }

    puts("Listening on port 8080...");
    connfd = accept(srv, (struct sockaddr*)&addr, &addrlen);
    if (connfd < 0) { perror("accept"); close(srv); exit(1); }
    printf("Client connected\n");

    pthread_t rtid, stid;
    pthread_create(&rtid, NULL, recv_thread, NULL);
    pthread_create(&stid, NULL, send_thread, NULL);

    pthread_join(stid, NULL);
    pthread_join(rtid, NULL);

    close(connfd);
    close(srv);
    return 0;
}
