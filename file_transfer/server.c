// server.c
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE (1024 * 16)
#define MAX_FILENAME_LEN 4096

static int server_fd = -1;

static void DieWithError(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void handle_sigint(int signum) {
    (void)signum;
    if (server_fd >= 0) close(server_fd);
    fprintf(stderr, "Server shutting down\n");
    _exit(0);
}

/* Portable 64-bit network-to-host */
static uint64_t ntoh64(uint64_t net) {
#if defined(be64toh)
    return be64toh(net);
#elif defined(__APPLE__)
    return OSSwapBigToHostInt64(net);
#else
    uint32_t hi = ntohl((uint32_t)(net >> 32));
    uint32_t lo = ntohl((uint32_t)(net & 0xffffffffu));
    return (((uint64_t)lo) << 32) | hi;
#endif
}

static ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    char *p = buf;
    while (total < len) {
        ssize_t n = recv(fd, p + total, len - total, 0);
        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}
static void receive_file(int client_fd) {
    uint32_t name_len_net;
    uint32_t name_len;
    char *file_name = NULL;
    uint64_t filesize_net, filesize;
    char buffer[BUFFER_SIZE];
    uint64_t received = 0;

    if (recv_all(client_fd, &name_len_net, sizeof(name_len_net)) <= 0)
        DieWithError("recv filename length");

    name_len = ntohl(name_len_net);
    if (name_len == 0 || name_len > MAX_FILENAME_LEN)
        DieWithError("invalid filename length");

    file_name = malloc(name_len + 1);
    if (!file_name) DieWithError("malloc");

    if (recv_all(client_fd, file_name, name_len) <= 0) DieWithError("recv filename");
    file_name[name_len] = '\0';

    if (recv_all(client_fd, &filesize_net, sizeof(filesize_net)) <= 0)
        DieWithError("recv filesize");

    filesize = ntoh64(filesize_net);

    /* build path: directory of the running binary + filename */
    char *save_path = NULL;
#if defined(PATH_MAX)
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir_end = strrchr(exe_path, '/');
        if (dir_end) *(dir_end + 1) = '\0'; /* keep trailing slash */
        size_t need = strlen(exe_path) + strlen(file_name) + 1;
        save_path = malloc(need + 1);
        if (!save_path) DieWithError("malloc");
        snprintf(save_path, need + 1, "%s%s", exe_path, file_name);
    } else {
        save_path = strdup(file_name);
        if (!save_path) DieWithError("malloc");
    }
#else
    save_path = strdup(file_name);
    if (!save_path) DieWithError("malloc");
#endif

    FILE *fp = fopen(save_path, "wb");
    if (!fp) {
        free(save_path);
        DieWithError("fopen");
    }

    while (received < filesize) {
        size_t to_read = (size_t)((filesize - received) > BUFFER_SIZE ? BUFFER_SIZE : (filesize - received));
        ssize_t n = recv_all(client_fd, buffer, to_read);
        if (n <= 0) {
            fclose(fp);
            free(save_path);
            DieWithError("recv file data");
        }
        size_t wrote = fwrite(buffer, 1, (size_t)n, fp);
        if (wrote != (size_t)n) {
            fclose(fp);
            free(save_path);
            DieWithError("fwrite");
        }
        received += (uint64_t)n;
    }

    fclose(fp);
    printf("Received file '%s' (%" PRIu64 " bytes)\n", save_path, filesize);
    free(save_path);
    free(file_name);
}


int main(void) {
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);

    signal(SIGINT, handle_sigint);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) DieWithError("socket");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        DieWithError("setsockopt");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        DieWithError("bind");

    if (listen(server_fd, 16) < 0) DieWithError("listen");

    printf("Server listening on port %d\n", PORT);

    for (;;) {
        int client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &clientaddr_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            DieWithError("accept");
        }

        char addrstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addrstr, sizeof(addrstr));
        printf("Client connected: %s:%d\n", addrstr, ntohs(clientaddr.sin_port));

        receive_file(client_fd);

        close(client_fd);
        printf("Client disconnected: %s\n", addrstr);
    }

    close(server_fd);
    return 0;
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

// #define PORT 12345
// #define BUF  (1024*10)

// static void die(const char *s){ perror(s); exit(1); }

// static ssize_t recv_all(int fd, void *buf, size_t len){
//     size_t t=0;
//     while(t<len){
//         ssize_t r = recv(fd, (char*)buf + t, len - t, 0);
//         if(r<=0) return r;
//         t += r;
//     }
//     return t;
// }

// int main(void){
//     int srv = socket(AF_INET, SOCK_STREAM, 0); if(srv<0) die("socket");
//     int opt = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//     struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY };
//     if(bind(srv,(struct sockaddr*)&a,sizeof(a))<0) die("bind");
//     if(listen(srv,10)<0) die("listen");
//     printf("listening %d\n", PORT);

//     int c = accept(srv, NULL, NULL); if(c<0) die("accept");

//     uint32_t name_len_be; if(recv_all(c,&name_len_be,sizeof(name_len_be))<=0) die("recv");
//     uint32_t name_len = ntohl(name_len_be);
//     if(name_len==0 || name_len>4096) die("bad name len");

//     char *name = malloc(name_len+1); if(!name) die("malloc");
//     if(recv_all(c,name,name_len)<=0) die("recv name"); name[name_len]=0;

//     uint64_t fs_be; if(recv_all(c,&fs_be,sizeof(fs_be))<=0) die("recv size");
//     uint64_t filesize = be64toh(fs_be);

//     FILE *f = fopen(name,"wb"); if(!f) die("fopen");
//     free(name);

//     char buf[BUF];
//     uint64_t got = 0;
//     while(got < filesize){
//         size_t want = (filesize - got) > BUF ? BUF : (size_t)(filesize - got);
//         ssize_t r = recv_all(c, buf, want);
//         if(r<=0) die("recv data");
//         fwrite(buf,1,r,f);
//         got += r;
//     }
//     fclose(f);
//     close(c);
//     close(srv);
//     printf("received %llu bytes\n", (unsigned long long)filesize);
//     return 0;
// }

