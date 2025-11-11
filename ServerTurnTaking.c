#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF 1024

int main(void) {
    int srv, cli;
    struct sockaddr_in addr;
    socklen_t alen = sizeof(addr);
    char buf[BUF];

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(srv); return 1; }
    if (listen(srv, 1) < 0) { perror("listen"); close(srv); return 1; }

    printf("Waiting for client on port %d...\n", PORT);
    cli = accept(srv, (struct sockaddr*)&addr, &alen);
    if (cli < 0) { perror("accept"); close(srv); return 1; }
    printf("Client connected\n");

    for (;;) {
        /* SERVER'S TURN: send a line */
        printf("You: ");
        if (!fgets(buf, BUF, stdin)) break;
        size_t len = strnlen(buf, BUF);
        if (len == 0) continue;
        if (send(cli, buf, len, 0) != (ssize_t)len) break;
        if (strncmp(buf, "exit", 4) == 0) break;

        /* CLIENT'S TURN: receive a line */
        ssize_t n = recv(cli, buf, BUF-1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("Client: %s", buf);
        if (strncmp(buf, "exit", 4) == 0) break;
    }

    close(cli);
    close(srv);
    printf("Server exiting\n");
    return 0;
}
