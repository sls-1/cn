// file_client.c
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE (1024 * 16)

static void DieWithError(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static ssize_t send_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const unsigned char *p = buf;
    while (total < len) {
        ssize_t n = send(fd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0;
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static off_t get_file_size(FILE *fp) {
#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
    if (fseeko(fp, 0, SEEK_END) != 0) return -1;
    off_t s = ftello(fp);
    rewind(fp);
    return s;
#else
    /* fallback */
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    long s = ftell(fp);
    rewind(fp);
    return (off_t)s;
#endif
}

static void send_file(int sockfd, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) DieWithError("fopen");

    off_t fsize = get_file_size(fp);
    if (fsize < 0) {
        fclose(fp);
        DieWithError("determine file size");
    }
    uint64_t filesize = (uint64_t)fsize;

    uint32_t name_len = (uint32_t)strlen(filename);
    if (name_len == 0) {
        fclose(fp);
        DieWithError("empty filename");
    }

    uint32_t name_len_be = htonl(name_len);
    if (send_all(sockfd, &name_len_be, sizeof(name_len_be)) != sizeof(name_len_be)) {
        fclose(fp);
        DieWithError("send filename length");
    }

    if (send_all(sockfd, filename, name_len) != (ssize_t)name_len) {
        fclose(fp);
        DieWithError("send filename");
    }

    uint64_t filesize_be = htobe64(filesize);
    if (send_all(sockfd, &filesize_be, sizeof(filesize_be)) != sizeof(filesize_be)) {
        fclose(fp);
        DieWithError("send filesize");
    }

    char buffer[BUFFER_SIZE];
    size_t n;
    uint64_t sent = 0;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send_all(sockfd, buffer, n) != (ssize_t)n) {
            fclose(fp);
            DieWithError("send file data");
        }
        sent += n;
    }

    if (ferror(fp)) {
        fclose(fp);
        DieWithError("fread");
    }

    fclose(fp);
    printf("File '%s' sent: %" PRIu64 " bytes\n", filename, sent);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    const char *port_str = argv[2];
    const char *filename = argv[3];

    int port = atoi(port_str);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "invalid port\n");
        return EXIT_FAILURE;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) DieWithError("socket");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, server_ip, &serveraddr.sin_addr) != 1) {
        close(sockfd);
        DieWithError("inet_pton");
    }

    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        close(sockfd);
        DieWithError("connect");
    }

    send_file(sockfd, filename);

    close(sockfd);
    return EXIT_SUCCESS;
}



// #define _GNU_SOURCE
// #include <arpa/inet.h>
// #include <endian.h>
// #include <netinet/in.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <unistd.h>

// #define BUF  (1024*10)

// static void die(const char *s){ perror(s); exit(1); }

// static ssize_t send_all(int fd, const void *buf, size_t len){
//     size_t t=0;
//     while(t<len){
//         ssize_t s = send(fd, (const char*)buf + t, len - t, 0);
//         if(s<=0) return s;
//         t += s;
//     }
//     return t;
// }

// int main(int argc, char **argv){
//     if(argc!=4){ fprintf(stderr,"Usage: %s <host> <port> <file>\n",argv[0]); return 1; }

//     const char *host = argv[1]; int port = atoi(argv[2]); const char *file = argv[3];

//     FILE *f = fopen(file,"rb"); if(!f) die("fopen");
//     fseek(f,0,SEEK_END); uint64_t filesize = ftell(f); rewind(f);

//     int sfd = socket(AF_INET, SOCK_STREAM, 0); if(sfd<0) die("socket");
//     struct sockaddr_in a; memset(&a,0,sizeof(a));
//     a.sin_family = AF_INET; a.sin_port = htons(port);
//     if(inet_pton(AF_INET, host, &a.sin_addr) <= 0) die("inet_pton");
//     if(connect(sfd,(struct sockaddr*)&a,sizeof(a))<0) die("connect");

//     uint32_t name_len = strlen(file);
//     uint32_t name_len_be = htonl(name_len);
//     if(send_all(sfd,&name_len_be,sizeof(name_len_be))!=sizeof(name_len_be)) die("send");
//     if(send_all(sfd,file,name_len)!=(ssize_t)name_len) die("send name");

//     uint64_t fs_be = htobe64(filesize);
//     if(send_all(sfd,&fs_be,sizeof(fs_be))!=sizeof(fs_be)) die("send size");

//     char buf[BUF]; size_t n;
//     while((n=fread(buf,1,sizeof(buf),f))>0){
//         if(send_all(sfd,buf,n)!=(ssize_t)n) die("send data");
//     }

//     fclose(f);
//     close(sfd);
//     printf("sent %llu bytes\n",(unsigned long long)filesize);
//     return 0;
// }
