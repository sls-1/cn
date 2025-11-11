#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];
    socklen_t addr_len = sizeof(server_addr);
    char *msg = "Hello from client";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Message sent to server\n");

    memset(buffer, 0, sizeof(buffer));
    recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
    printf("Server: %s\n", buffer);

    close(sockfd);
    return 0;
}
