#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 255

void DieWithError(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Server IP> <Message> <Port>\n", argv[0]);
        exit(1);
    }

    int clientSock;
    struct sockaddr_in servAddr;
    char *servIP = argv[1];
    char *msg = argv[2];
    unsigned short servPort = atoi(argv[3]);
    socklen_t servLen = sizeof(servAddr);
    char buffer[BUFSIZE + 1];
    int recvSize;

    if ((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(servIP);
    servAddr.sin_port = htons(servPort);

    if (sendto(clientSock, msg, strlen(msg), 0,
               (struct sockaddr *)&servAddr, servLen) != (int)strlen(msg))
        DieWithError("sendto() failed");

    if ((recvSize = recvfrom(clientSock, buffer, BUFSIZE, 0,
                             (struct sockaddr *)&servAddr, &servLen)) < 0)
        DieWithError("recvfrom() failed");

    buffer[recvSize] = '\0';
    printf("Received back from server: %s\n", buffer);

    close(clientSock);
    return 0;
}

