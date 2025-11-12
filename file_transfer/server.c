#include <stdio.h>     
#include <stdlib.h>    
#include <string.h>    
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096
#define FILENAME_MAX_LEN 256
#define MODE_LEN 10

void die_with_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_upload(int client_sock, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) die_with_error("fopen() failed");

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;

    printf("Receiving file '%s'...\n", filename);

    while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buffer, 1, bytes_received, file) != bytes_received)
            die_with_error("fwrite() failed");
        total_bytes += bytes_received;
    }

    if (bytes_received < 0) die_with_error("recv() failed");

    fclose(file);
    printf("File upload complete. Total bytes received: %ld\n", total_bytes);
}

void handle_download(int client_sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "fopen() failed for file: %s\n", filename);
        close(client_sock);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_bytes = 0;

    printf("Sending file '%s'...\n", filename);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        ssize_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t bytes_sent = send(client_sock, buffer + total_sent, bytes_read - total_sent, 0);
            if (bytes_sent < 0) {
                fprintf(stderr, "send() failed.\n");
                fclose(file);
                close(client_sock);
                return;
            }
            total_sent += bytes_sent;
        }
        total_bytes += total_sent;
    }

    fclose(file);
    printf("File send complete. Total bytes sent: %ld\n", total_bytes);
    close(client_sock);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) die_with_error("socket() failed");

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die_with_error("setsockopt() failed");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die_with_error("bind() failed");

    if (listen(server_sock, 5) < 0)
        die_with_error("listen() failed");

    printf("Server listening on port %d...\n", port);

    for (;;) {
        printf("Waiting for a client to connect...\n");
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept() failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s\n", client_ip);

        char mode_buffer[MODE_LEN];
        char filename_buffer[FILENAME_MAX_LEN];

        ssize_t recv_len = recv(client_sock, mode_buffer, MODE_LEN, 0);
        if (recv_len <= 0) {
            fprintf(stderr, "Failed to receive mode from client\n");
            close(client_sock);
            continue;
        }
        mode_buffer[MODE_LEN - 1] = '\0';

        recv_len = recv(client_sock, filename_buffer, FILENAME_MAX_LEN, 0);
        if (recv_len <= 0) {
            fprintf(stderr, "Failed to receive filename from client\n");
            close(client_sock);
            continue;
        }
        filename_buffer[FILENAME_MAX_LEN - 1] = '\0';

        printf("Client requested mode: '%s', Filename: '%s'\n", mode_buffer, filename_buffer);

        if (strcmp(mode_buffer, "upload") == 0)
            handle_upload(client_sock, filename_buffer);
        else if (strcmp(mode_buffer, "download") == 0)
            handle_download(client_sock, filename_buffer);
        else
            fprintf(stderr, "Unknown mode from client: %s\n", mode_buffer);

        if (strcmp(mode_buffer, "upload") == 0)
            close(client_sock);

        printf("Client request finished. Waiting for new connection.\n");
    }

    close(server_sock);
    return 0;
}
