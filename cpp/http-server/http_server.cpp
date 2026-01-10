#include <iostream>
#include <string_view>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>  
#include <vector> 

// constexpr const char* CRLF = "\r\n";
// constexpr const char* SP = " ";

// constexpr std::string_view WEB_ROOT = "./www";

typedef struct {
	std::string_view method;
	std::string_view uri;
	std::string_view version;
} http_request_line;

typedef enum http_status {
	HTTP_RES_OK = 200,
	HTTP_RES_BAD_REQUEST = 400,
	HTTP_RES_NOT_FOUND = 404,
	HTTP_RES_INTERNAL_SERVER_ERR = 500
} http_status;

const char* http_status_to_string(http_status status) {
	switch (status) {
	case HTTP_RES_OK:
		return "OK";
	case HTTP_RES_BAD_REQUEST:
		return "Bad request";
	case HTTP_RES_INTERNAL_SERVER_ERR:
		return "Internal server error";
	case HTTP_RES_NOT_FOUND:
		return "Not found";
	default:
		return "Unknown";
	}
}

const int PORT = 9999;

int main(void) {
	int rc = 0;
	struct sockaddr_in bind_addr;
	int tcp_socket = 0;
	int client_socket = 0;
	int enabled = true;

	/*
	fs_metadata web_root_metadata = get_fs_metadata(WEB_ROOT);
	if (!web_root_metadata.exists) {
		mkdir(WEB_ROOT.data, S_IEXEC | S_IWRITE | S_IREAD | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	*/

	tcp_socket = socket(
		AF_INET,
		SOCK_STREAM,
		0
	);

	if (tcp_socket < 0) {
		std::cerr << "socket()" << std::endl;
		return 1;
	}
	std::cout << "socket creation succeeded" << std::endl;

	/* Ignore failure and use SO_REUSEADDR (not production ready) */
	rc = setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

	bind_addr.sin_port = htons(PORT);
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	rc = bind(
		tcp_socket,
		(const struct sockaddr*)&bind_addr,
		sizeof(bind_addr)
	);
	if (rc < 0) {
		std::cerr << "bind()" << std::endl;
		close(tcp_socket);
		return 1;
	}
	std::cout << "bind succeeded" << std::endl;

	rc = listen(
		tcp_socket,
		SOMAXCONN
	);
	if (rc < 0) {
		std::cerr << "listen()" << std::endl;
		close(tcp_socket);
		return 1;
	}
	std::cout << "listening on http://localhost:" << PORT << "/" << std::endl;

	for (;;) {
		std::cout << "waiting for connections" << std::endl;
		client_socket = accept(tcp_socket, NULL, NULL);

		std::cout << "got a connection" << std::endl;
		// rc = handle_client(client_socket);
	}

	return 0;
}