#include "DieWithError.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]){
if (argc != 3){
	
	fprintf(stderr, "Usage: <IP> <PORT>\n");
		return 1;
}
char *servIP = argv[1];
//char *msg = argv[3];
while(1){
	char msg[1024];
	printf("Enter The Message");
	fgets(msg,sizeof(msg),stdin);

	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0) DieWithError("socket() failed");
	struct sockaddr_in servAddr;
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP);
	servAddr.sin_port = htons((unsigned short)atoi(argv[2]));
	if(connect(sock,(struct sockaddr *)&servAddr,sizeof(servAddr))<0)DieWithError("connect() failed");
	size_t msgLen = strlen(msg);
	if(send(sock, msg, msgLen, 0) != (ssize_t)msgLen)DieWithError("send() diff number of bytes.");
	
	char buf[256];
	ssize_t r;
	if((r = recv(sock, buf ,sizeof(buf)-1,0))< 0)DieWithError("recv() failed");
	buf[r]=0;
	printf("Recevied: %s \n",buf);
	close(sock);
}	
return 0;
}	
