#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h> 

#define BUFFER_SIZE 4096

void die_with_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void *receive_messages(void *socket_desc) {
    int server_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(server_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; 
        printf("%s", buffer);
        fflush(stdout); 
    }

    if (bytes_received == 0) {
        printf("Server disconnected. Press Enter to exit.\n");
    } else {
        perror("recv() failed");
    }

    printf("Receive thread exiting.\n");

    exit(0); 
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int server_sock;
    struct sockaddr_in server_addr;
    pthread_t receive_thread_id;
    char buffer[BUFFER_SIZE];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        die_with_error("socket() failed");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        die_with_error("inet_pton() failed");
    }

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die_with_error("connect() failed");
    }

    printf("Connected to server %s:%d. You can start chatting.\n", server_ip, port);

    if (pthread_create(&receive_thread_id, NULL, receive_messages, (void *)&server_sock) < 0) {
        die_with_error("pthread_create() failed");
    }

    printf("Type your message and press Enter to send. Type 'exit' to quit.\n");
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break; 
        }
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        if (send(server_sock, buffer, strlen(buffer), 0) < 0) {
            perror("send() failed");
            break;
        }
    }

    printf("Closing connection...\n");
    close(server_sock);

    printf("Client shutting down.\n");
    return 0;
}