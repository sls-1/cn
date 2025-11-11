#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF 1024

int main(void) {
    int s;
    struct sockaddr_in serv;
    
    char buf[BUF];

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr) <= 0) { perror("inet_pton"); close(s); return 1; }

    if (connect(s, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("connect"); close(s); return 1; }
    printf("Connected to server\n");

    for (;;) {
        /* SERVER'S TURN: receive a line first */
        ssize_t n = recv(s, buf, BUF-1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("Server: %s", buf);
        if (strncmp(buf, "exit", 4) == 0) break;

        /* CLIENT'S TURN: send a line */
        printf("You: ");
        if (!fgets(buf, BUF, stdin)) break;
        size_t len = strnlen(buf, BUF);
        if (len == 0) continue;
        if (send(s, buf, len, 0) != (ssize_t)len) break;
        if (strncmp(buf, "exit", 4) == 0) break;
    }

    close(s);
    printf("Client exiting\n");
    return 0;
}
