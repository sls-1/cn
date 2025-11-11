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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(1);
    }

    int servSock;
    struct sockaddr_in servAddr, clntAddr;
    unsigned short servPort = atoi(argv[1]);
    socklen_t clntLen;
    char buffer[BUFSIZE + 1];
    int recvSize;

    if ((servSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieWithError("bind() failed");

    for (;;) {
        clntLen = sizeof(clntAddr);
        if ((recvSize = recvfrom(servSock, buffer, BUFSIZE, 0,
                                 (struct sockaddr *)&clntAddr, &clntLen)) < 0)
            DieWithError("recvfrom() failed");

        buffer[recvSize] = '\0';
        printf("Received from %s:%d: %s\n",
               inet_ntoa(clntAddr.sin_addr),
               ntohs(clntAddr.sin_port),
               buffer);

        if (sendto(servSock, buffer, recvSize, 0,
                   (struct sockaddr *)&clntAddr, clntLen) != recvSize)
            DieWithError("sendto() failed");
    }
}

