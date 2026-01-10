#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "string_ops.h"
#include "fs.h"

#define CRLF "\r\n"
#define SP " "

const string WEB_ROOT = STRING_FROM_CSTR("./www/")

typedef struct {
    string method;
    string uri;
    string version;
} http_request_line;

typedef enum http_status {
    HTTP_RES_OK = 200,
    HTTP_RES_BAD_REQUEST = 400,
    HTTP_RES_NOT_FOUND = 404,
    HTTP_RES_INTERNAL_SERVER_ERR = 500
} http_status;

static const char* http_status_to_string(http_status status) {
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


static http_request_line http_request_line_init(void) {
    http_request_line line;
    memset(&line, 0, sizeof(line));
    return line;
}

static http_status parse_request_line(http_request_line* request_line, const char* buf, size_t len){
    if (!buf || !request_line) {
        return HTTP_RES_INTERNAL_SERVER_ERR;
    }

    string_splits components = split_string(buf, len, SP);

    if (components.count != 3) {
        printf("ERROR: invalid request line: expected 3 components, got %zu\n", components.count);
        return HTTP_RES_BAD_REQUEST;
    }

    request_line->method.data = components.splits[0].data;
    request_line->method.len = components.splits[0].len;
    request_line->uri.data = components.splits[1].data;
    request_line->uri.len = components.splits[1].len;
    request_line->version.data = components.splits[2].data;
    request_line->version.len = components.splits[2].len;

    free_splits(&components);
    return HTTP_RES_OK;
}

static string generate_http_response(char* buf, size_t buf_len, http_status status, size_t body_len) {
    string response;
    response.len = 0;
    memset(buf, 0, buf_len);

    response.len += sprintf(buf, "%s %d %s" CRLF, "HTTP/1.0", status, http_status_to_string(status));
    response.len += sprintf(buf + response.len, "Content-Length: %zu" CRLF, body_len);
    response.len += sprintf(buf + response.len, CRLF);
    response.data = buf;

    return response;
}

static bool send_http_response(int socket, string header, string body) {
    ssize_t n = send(socket, header.data, header.len, MSG_MORE);

    if (n < 0) {
        perror("send()");
        return false;
    } else if (n == 0) {
        fprintf(stderr, "send() returned 0\n");
        return false;
    }

    n = send(socket, body.data, body.len, 0);
    return true;
}

static string not_found = STRING_FROM_CSTR("<p>Error 404: not found</p>")

bool serve_http_file(int socket, string filename) {
    char buf[64];
    char filename_buf[PATH_MAX];
    int in_fd = -1;
    string header;
    ssize_t result = 0;
    bool return_value = true;
    size_t sent = 0;
    off_t sendfile_offset = 0;

    memset(filename_buf, 0, sizeof(filename_buf));
    memcpy(filename_buf, WEB_ROOT.data, WEB_ROOT.len);
    memcpy(filename_buf + WEB_ROOT.len - 1, filename.data, filename.len);

    fs_metadata file_metadata = get_fs_metadata(string_from_cstr(filename_buf));
    if (!file_metadata.exists) {
        (void)send_http_response(socket,
        generate_http_response(buf, sizeof(buf), HTTP_RES_NOT_FOUND, not_found.len),
        not_found);
        return_value = false;
        if (in_fd != -1) {
            close(in_fd);
        }
        return return_value;
    }

    header = generate_http_response(buf, sizeof(buf), HTTP_RES_OK, file_metadata.size);

    ssize_t n = send(socket, header.data, header.len, MSG_MORE);
    if (n < 0) {
        perror("send()");
        return_value = false;
        if (in_fd != -1) {
            close(in_fd);
        }
        return return_value;
    } else if (n == 0) {
        fprintf(stderr, "send() returned 0\n");
        return_value = false;
        if (in_fd != -1) {
            close(in_fd);
        }
        return return_value;
    }

    in_fd = open(filename_buf, O_RDONLY);
    if (in_fd < 0) {
        (void)send_http_response(socket,
        generate_http_response(buf, sizeof(buf), HTTP_RES_NOT_FOUND, not_found.len),
        not_found);
        return_value = false;
        if (in_fd != -1) {
            close(in_fd);
        }
        return return_value;
    }
    while (sent < file_metadata.size) {
        result = sendfile(socket, in_fd, &sendfile_offset, file_metadata.size);
        if (result < 0) {
            printf("sendfile()");
            (void)send_http_response(socket,
            generate_http_response(buf, sizeof(buf), HTTP_RES_NOT_FOUND, not_found.len),
            not_found);
            return_value = false;
            if (in_fd != -1) {
                close(in_fd);
            }
            return return_value;
        }
        sent += result;
    }

    if (in_fd != -1) {
        close(in_fd);
    }
    return return_value;
}

int handle_client(int client_socket) {
	ssize_t n = 0;
	char buf[1024];

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

		printf("REQUEST:\n%s", buf);

        string_splits lines = split_string(buf, n, CRLF);

        if (lines.count < 1) {
            printf("ERROR: empty requests\n");
            return -1;
        }

		http_request_line request_line = http_request_line_init();
		http_status result = parse_request_line(&request_line, lines.splits[0].data, lines.splits[1].len);
        free_splits(&lines);
        if (result != HTTP_RES_OK) {
            printf("failed to parse request line\n");
            return -1;
        }

        string route_root = string_from_cstr("/");
        if (string_equal(&request_line.uri, &route_root)) {
            if (!serve_http_file(client_socket, string_from_cstr("index.html"))) {
                return -1;
            }
        } else {
            send_http_response(client_socket,
            generate_http_response(buf, sizeof(buf), HTTP_RES_NOT_FOUND, not_found.len),
            not_found);
            return -1;
        }
		
        close(client_socket);
        break;
	}

	printf("\n---\n");

	return 0;
}

const int PORT = 9999;

int main(void) {
	int rc = 0;
    struct sockaddr_in bind_addr;
	int tcp_socket = 0;
	int client_socket = 0;
	int enabled = true;

    fs_metadata web_root_metadata = get_fs_metadata(WEB_ROOT);
    if (!web_root_metadata.exists) {
        mkdir(WEB_ROOT.data, S_IEXEC | S_IWRITE | S_IREAD | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

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

    bind_addr.sin_port = htons(PORT);
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
	printf("listening on http://localhost:%d/\n", PORT);

	for (;;) {
		printf("waiting for connections\n");
		client_socket = accept(tcp_socket, NULL, NULL);

		printf("got a connection\n");
		rc = handle_client(client_socket);
	}

	return 0;
}
