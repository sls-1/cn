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
void handle_upload(int server_sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        die_with_error("fopen() failed. File may not exist.");
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_bytes = 0;

    printf("Sending file '%s'...\n", filename);

    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        ssize_t total_sent = 0;

        while (total_sent < bytes_read) {
            ssize_t bytes_sent = send(server_sock, buffer + total_sent, bytes_read - total_sent, 0);
            if (bytes_sent < 0) {
                die_with_error("send() failed");
            }
            total_sent += bytes_sent;
        }
        total_bytes += total_sent;
    }

    fclose(file);


    if (shutdown(server_sock, SHUT_WR) < 0) {
        die_with_error("shutdown() failed");
    }
    
    printf("File send complete. Total bytes sent: %ld\n", total_bytes);
}

void handle_download(int server_sock, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        die_with_error("fopen() failed");
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;

    printf("Receiving file '%s'...\n", filename);

    while ((bytes_received = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buffer, 1, bytes_received, file) != bytes_received) {
            die_with_error("fwrite() failed");
        }
        total_bytes += bytes_received;
    }

    if (bytes_received < 0) {
        die_with_error("recv() failed");
    }

    fclose(file);
    if (total_bytes == 0) {
      
        printf("File download complete, but 0 bytes received.\n");
        printf("This may mean the file did not exist on the server.\n");
    } else {
        printf("File download complete. Total bytes received: %ld\n", total_bytes);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <mode> <filename>\n", argv[0]);
        fprintf(stderr, "  Modes:\n");
        fprintf(stderr, "    'upload'   - Sends <filename> to the server.\n");
        fprintf(stderr, "    'download' - Requests <filename> from server, saves it as <filename>.\n");
        fprintf(stderr, "  Example (upload):   %s 127.0.0.1 8080 upload my_video.mp4\n", argv[0]);
        fprintf(stderr, "  Example (download): %s 127.0.0.1 8080 download server_file.dat\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    const char *mode = argv[3];
    const char *filename = argv[4];

    if (strcmp(mode, "upload") != 0 && strcmp(mode, "download") != 0) {
        fprintf(stderr, "Invalid mode. Use 'upload' or 'download'.\n");
        exit(EXIT_FAILURE);
    }
    
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }
    
    if (strlen(filename) >= FILENAME_MAX_LEN) {
        fprintf(stderr, "Filename is too long (max %d chars).\n", FILENAME_MAX_LEN - 1);
        exit(EXIT_FAILURE);
    }


    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        die_with_error("socket() failed");
    }

     memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // Network byte order
   if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        die_with_error("inet_pton() failed for invalid address");
    }

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die_with_error("connect() failed");
    }

    printf("Connected to server %s:%d\n", server_ip, port);

     char mode_buffer[MODE_LEN] = {0};
    char filename_buffer[FILENAME_MAX_LEN] = {0};
    
     strncpy(mode_buffer, mode, MODE_LEN - 1);
    if (send(server_sock, mode_buffer, MODE_LEN, 0) < 0) {
        die_with_error("send(mode) failed");
    }

    strncpy(filename_buffer, filename, FILENAME_MAX_LEN - 1);
    if (send(server_sock, filename_buffer, FILENAME_MAX_LEN, 0) < 0) {
        die_with_error("send(filename) failed");
    }


     if (strcmp(mode, "upload") == 0) {
        handle_upload(server_sock, filename);
    } else { 
         handle_download(server_sock, filename);
    }

     close(server_sock);

    return 0;
}