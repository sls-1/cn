#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("Error Try Again");
		return 1;
	}
	char *servIP = argv[1];
	char *msg = argv[3];
	unsigned short servPort = (unsigned short)atoi(argv[2]);

	while (1)
	{
		int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0)
		{
			printf("Error");
			return -1;
		}
		struct sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = inet_addr(servIP);
		servAddr.sin_port = htons(servPort);
		if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
		{
			printf("Error");
			return -1;
		}
		size_t msgLen = strlen(msg);
		if (send(sock, msg, msgLen, 0) != (ssize_t)msgLen)
		{
			printf("Error");
			return -1;
		}
		char buf[256];
		ssize_t r;
		if ((r = recv(sock, buf, sizeof(buf) - 1, 0)) < 0)
		{
			printf("Error");
			return -1;
		}
		buf[r] = 0;
		printf("Recevied: %s", buf);
		close(sock);
	}
	return 0;
}
