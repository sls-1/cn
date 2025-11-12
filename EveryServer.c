#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      
#include <arpa/inet.h>   
#include <sys/socket.h>  
#include <pthread.h>     
#include <stdint.h>      

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *recv_thread(void *arg) {
    int sock = (intptr_t)arg;
    char buffer[BUFFER_SIZE];
    int valread;
    int id = sock; 

    while ((valread = recv(sock, buffer, BUFFER_SIZE - 1,0)) > 0) {
        buffer[valread] = '\0';
        printf("%d: %s\n", id, buffer);
    }

    printf("Client %d disconnected.\n", id);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sock) {
            client_sockets[i] = 0; 
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(sock);
    pthread_exit(NULL);
}

void *send_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    int valread;

    while ((valread = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[valread] = '\0';

        int target_id;
        char message[BUFFER_SIZE];

        if (sscanf(buffer, "%d:%[^\n]", &target_id, message) == 2) {
            int found = 0;
            
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == target_id) {
                    send(target_id, message, strlen(message), 0);
                    printf("Server -> %d: %s\n", target_id, message);
                    found = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            if (!found) {
                printf("Client ID %d not found.\n", target_id);
            }
        } else {
            puts("Server: Invalid format. Use '<id>:<message>'.");
        }
    }
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    socklen_t client_addr_len = sizeof(client_addr);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    puts("Waiting for connections ...");

    pthread_t stdin_thread;
    if (pthread_create(&stdin_thread, NULL, send_thread, NULL) != 0) {
        perror("pthread_create for stdin failed");
        exit(EXIT_FAILURE);
    }
    pthread_detach(stdin_thread); 

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("accept");
            continue; 
        }

        printf("New connection: ID is %d (IP: %s, Port: %d)\n", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_mutex_lock(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;

                pthread_t client_thread;
                if (pthread_create(&client_thread, NULL, recv_thread, (void*)(intptr_t)new_socket) != 0) {
                    perror("pthread_create for client failed");
                    client_sockets[i] = 0;
                } else {
                    pthread_detach(client_thread); 
                }
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

    } 

    close(server_fd);
    return 0;
}