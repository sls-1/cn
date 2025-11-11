/*
 * server.c
 * This program acts as a TCP server that can either:
 * 1. Receive (upload) a large file from a client.
 * 2. Send (download) a large file to a client.
 *
 * The client dictates the mode and filename over the socket.
 */

#include <stdio.h>      // For printf, fprintf, FILE, fopen, etc.
#include <stdlib.h>     // For exit, atexit, EXIT_FAILURE
#include <string.h>     // For memset
#include <unistd.h>     // For close, read, write
#include <arpa/inet.h>  // For INADDR_ANY, htons, etc.
#include <sys/socket.h> // For socket, bind, listen, accept
#include <netinet/in.h> // For struct sockaddr_in

#define BUFFER_SIZE 4096      // Use a 4KB buffer
#define FILENAME_MAX_LEN 256  // Max length for a filename
#define MODE_LEN 10           // Max length for mode string ("upload")

/*
 * Helper function to print an error message and exit.
 */
void die_with_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
 * Handles receiving a file from the client (upload).
 * The server reads from the client socket and writes to a file.
 */
void handle_upload(int client_sock, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        die_with_error("fopen() failed");
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;

    printf("Receiving file '%s'...\n", filename);

    // Loop until recv() returns 0 (connection closed) or -1 (error)
    while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buffer, 1, bytes_received, file) != bytes_received) {
            die_with_error("fwrite() failed");
        }
        total_bytes += bytes_received;
    }

    if (bytes_received < 0) {
        die_with_error("recv() failed");
    }

    fclose(file);
    printf("File upload complete. Total bytes received: %ld\n", total_bytes);
}

/*
 * Handles sending a file to the client (download).
 * The server reads from a file and writes to the client socket.
 */
void handle_download(int client_sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        // Don't die, just tell the server console
        fprintf(stderr, "fopen() failed for file: %s. File may not exist.\n", filename);
        // We can't send the file, so just close the connection.
        // The client's recv() will get 0 and know the transfer "finished" (with 0 bytes).
        close(client_sock);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_bytes = 0;

    printf("Sending file '%s'...\n", filename);

    // Read from file until end-of-file
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        ssize_t total_sent = 0;

        // Loop to ensure all bytes from the buffer are sent
        while (total_sent < bytes_read) {
            ssize_t bytes_sent = send(client_sock, buffer + total_sent, bytes_read - total_sent, 0);
            if (bytes_sent < 0) {
                // If send fails, the client probably disconnected
                fprintf(stderr, "send() failed. Client may have disconnected.\n");
                fclose(file);
                close(client_sock); // Ensure socket is closed
                return;
            }
            total_sent += bytes_sent;
        }
        total_bytes += total_sent;
    }

    fclose(file);
    printf("File send complete. Total bytes sent: %ld\n", total_bytes);
    
    // We close the client socket to signal the end of transmission
    // The client's recv() loop will then get a 0.
    close(client_sock);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        fprintf(stderr, "  Example: %s 8080\n", argv[0]);
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

    // 1. Create the server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        die_with_error("socket() failed");
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        die_with_error("setsockopt() failed");
    }

    // 2. Prepare the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(port);       // Network byte order

    // 3. Bind the socket to the address and port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die_with_error("bind() failed");
    }

    // 4. Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        die_with_error("listen() failed");
    }

    printf("Server listening on port %d...\n", port);
    
    // 5. Accept client connections in a loop
    for (;;) {
        printf("Waiting for a client to connect...\n");
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            // Don't die, just log the error and wait for the next one
            perror("accept() failed");
            continue; 
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s\n", client_ip);

        // --- Protocol Start ---
        char mode_buffer[MODE_LEN];
        char filename_buffer[FILENAME_MAX_LEN];

        // 6. Receive mode from client
        ssize_t recv_len = recv(client_sock, mode_buffer, MODE_LEN, 0);
        if (recv_len <= 0) {
            fprintf(stderr, "Failed to receive mode from client\n");
            close(client_sock);
            continue; // Wait for next client
        }
        mode_buffer[MODE_LEN - 1] = '\0'; // Ensure null-termination

        // 7. Receive filename from client
        recv_len = recv(client_sock, filename_buffer, FILENAME_MAX_LEN, 0);
        if (recv_len <= 0) {
            fprintf(stderr, "Failed to receive filename from client\n");
            close(client_sock);
            continue; // Wait for next client
        }
        filename_buffer[FILENAME_MAX_LEN - 1] = '\0'; // Ensure null-termination

        printf("Client requested mode: '%s', Filename: '%s'\n", mode_buffer, filename_buffer);

        // 8. Handle the client request based on mode
        if (strcmp(mode_buffer, "upload") == 0) {
            handle_upload(client_sock, filename_buffer);
        } else if (strcmp(mode_buffer, "download") == 0) {
            handle_download(client_sock, filename_buffer);
        } else {
            fprintf(stderr, "Unknown mode from client: %s\n", mode_buffer);
        }
        // --- Protocol End ---

        // 9. Close client socket (if not already closed by handler)
        // handle_download() closes its own socket to signal EOF.
        // handle_upload() relies on client's shutdown, so we close here.
        if (strcmp(mode_buffer, "upload") == 0) {
            close(client_sock);
        }
        printf("Client request finished. Waiting for new connection.\n");
    }

    close(server_sock); // Unreachable, but good practice
    return 0;
}