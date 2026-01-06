#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

int handle_client(int client_socket) {
	ssize_t n = 0;
	char buf[100];

	for (;;) {
		memset(buf, 0, sizeof(buf));

		n = read(client_socket, buf, sizeof(buf) - 1);
		if (n < 0) {
			perror("read(client)");
			return -1;
		}
		if (n == 0) {
			printf("connection closed without any errors\n");
			break;
		}

		printf("%s\n", buf);
	}

	printf("\n---\n");

	return 0;
}

int main(void) {
	int rc = 0;
    struct sockaddr_in bind_addr;
	int tcp_socket = 0;
	int client_socket = 0;
	int enabled = true;

    memset(&bind_addr, 0, sizeof(bind_addr));
	tcp_socket = socket(
		AF_INET,
		SOCK_STREAM,
		0
	);

    if (tcp_socket < 0) {
        perror("socket()");
        return 1;
    }
    printf("socket creation succeeded\n");

	/* Ignore failure and use SO_REUSEADDR (not production ready) */
	rc = setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

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
	if (rc < 0) {
		perror("listen()");
		close(tcp_socket);
		return 1;
	}
	printf("listen succeeded\n");

	for (;;) {
		printf("waiting for connections\n");
		client_socket = accept(tcp_socket, NULL, NULL);

		printf("got a connection\n");
		rc = handle_client(client_socket);
	}

	return 0;
}
