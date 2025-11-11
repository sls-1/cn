/* TCPEchoClient.c */
#include "DieWithError.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort>\n", argv[0]);
        return 1;
    }
    char *servIP = argv[1];
    // char *msg = argv[2];
    while (1)
    {
        char msg[1024];
        printf("Enter a message: ");
        scanf("%s", &msg);
        unsigned short servPort = (unsigned short)atoi(argv[2]);
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0)
            DieWithError("socket() failed");
        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = inet_addr(servIP);
        servAddr.sin_port = htons(servPort);
        if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
            DieWithError("connect() failed");
        size_t msgLen = strlen(msg);
        if (send(sock, msg, msgLen, 0) != (ssize_t)msgLen)
            DieWithError("send() sent different number of bytes");
        char buf[256];
        ssize_t r;
        if ((r = recv(sock, buf, sizeof(buf) - 1, 0)) < 0)
            DieWithError("recv() failed");
        buf[r] = 0;
        printf("Received: %s\n", buf);
        close(sock);
    }
    return 0;
}