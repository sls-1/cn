#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 1024

int main(void) {
    int srv, cli;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buf[BUF_SIZE];

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); close(srv); exit(EXIT_FAILURE); }
    if (listen(srv, 1) < 0) { perror("listen"); close(srv); exit(EXIT_FAILURE); }

    puts("Listening on port 8080...");
    cli = accept(srv, (struct sockaddr *)&addr, &addrlen);
    if (cli < 0) { perror("accept"); close(srv); exit(EXIT_FAILURE); }

    for (;;) {
        ssize_t n = recv(cli, buf, BUF_SIZE - 1, 0);
        if (n < 0) { perror("recv"); break; }
        if (n == 0) break; // client closed connection

        buf[n] = '\0';
        printf("From client: %s", buf);

        if (send(cli, buf, n, 0) != n) { perror("send"); break; }

        if (strncmp(buf, "exit", 4) == 0) {
            puts("Server Exit...");
            break;
        }
    }

    close(cli);
    close(srv);
    return 0;
}
