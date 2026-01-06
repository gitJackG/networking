#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void){
	int rc = 0;
    struct sockaddr_in bind_addr;
	int tcp_socket = 0;
	int client_socket = 0;

    memset(&bind_addr, 0, sizeof(bind_addr));
	tcp_socket = socket(
		AF_INET,
		SOCK_STREAM,
		0
	);

    if (tcp_socket < 0){
        perror("socket()");
        return 1;
    }
    printf("socket creation succeeded\n");

    bind_addr.sin_port = htons(9999);
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;

   	rc = bind(
        tcp_socket,
        (const struct sockaddr*)&bind_addr,
        sizeof(bind_addr)
    );
    if (rc < 0) {
        perror("bind()");
		close(tcp_socket);
        return 1;
    }
    printf("bind succeeded\n");

    rc = listen(
		tcp_socket,
		SOMAXCONN
		);
	if (rc < 0){
		perror("listen()");
		close(tcp_socket);
		return 1;
	}
	printf("listen succeeded\n");

	printf("waiting for connections\n");
	client_socket = accept(tcp_socket, NULL, NULL);

	printf("got a connection\n");

	return 0;
}
