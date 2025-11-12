// bidirectional, multi-client RELAY server using select()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h> 
#include <sys/time.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int max(int a, int b) {
    return (a > b) ? a : b;
}

void bridge_clients(int client1_fd, int client2_fd) {
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int valread;

    int max_fd = max(client1_fd, client2_fd);

    printf("Bridging clients %d and %d. Relay active.\n", client1_fd, client2_fd);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(client1_fd, &readfds);
        FD_SET(client2_fd, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            break; 
        }

        if (FD_ISSET(client1_fd, &readfds)) {
            valread = recv(client1_fd, buffer, BUFFER_SIZE, 0);
            if (valread > 0) {
                send(client2_fd, buffer, valread, 0);
            } else {
                printf("Client %d disconnected. Breaking bridge.\n", client1_fd);
                break;
            }
        }

        if (FD_ISSET(client2_fd, &readfds)) {
            valread = recv(client2_fd, buffer, BUFFER_SIZE, 0);
            if (valread > 0) {
                send(client1_fd, buffer, valread, 0);
            } else {
                printf("Client %d disconnected. Breaking bridge.\n", client2_fd);
                break; 
            }
        }
    } 

    close(client1_fd);
    close(client2_fd);
    printf("Bridge closed. Waiting for new pair...\n");
}

int main() {
    int server_fd, client1_fd, client2_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    printf("Relay server listening on port %d\n", PORT);

    while (1) {
        printf("Waiting for first client in pair...\n");
        if ((client1_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept1 failed");
            continue; 
        }
        printf("Client 1 connected (ID %d). Waiting for second client...\n", client1_fd);

        printf("Waiting for second client in pair...\n");
        if ((client2_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept2 failed");
            close(client1_fd); 
            continue; 
        }
        printf("Client 2 connected (ID %d).\n", client2_fd);
        bridge_clients(client1_fd, client2_fd);
    }

    close(server_fd);
    return 0;
}
