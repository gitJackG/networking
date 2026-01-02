#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void){
    int tcp_socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (tcp_socket == -1){
        perror("socket()");
        return 1;
    }
    printf("socket creation succeeded\n");

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_port = htons(9999);
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    int rc = bind(
        tcp_socket,
        (const struct sockaddr*)&bind_addr,
        sizeof(bind_addr)
    );
    if (rc < 0) {
        perror("bind()");
        return 1;
    }
    printf("bind succeeded\n");
}
