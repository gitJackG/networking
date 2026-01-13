#include <string>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define CRLF "\r\n"
#define SP " "

static const std::string NOT_FOUND_BODY = "<p>Error 404: not found</p>";
static const std::string WEB_ROOT = "./www/";

typedef struct
{
    std::string method;
    std::string uri;
    std::string version;
} http_request_line;

typedef enum
{
    HTTP_RES_OK = 200,
    HTTP_RES_BAD_REQUEST = 400,
    HTTP_RES_NOT_FOUND = 404,
    HTTP_RES_INTERNAL_SERVER_ERR = 500,
} http_status_code;

std::string http_status_to_string(http_status_code status);
int handle_connection(int socket);
int handle_request(int socket);
http_status_code parse_request(const std::string& request, http_request_line& request_line);
std::string generate_response_header(http_status_code status, size_t file_length);
size_t send_response_header(int socket, const std::string& response_header, const std::string& response_header_body);
size_t serve_file(int socket, const std::string& filename);

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "socket()" << std::endl;
        return 1;
    }

    int rc = 0;
    int enabled = 1;

    rc = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    if (rc < 0)
    {
        std::cerr << "setsockopt()" << std::endl;
        close(serverSocket);
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    rc = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (rc < 0)
    {
        std::cerr << "bind()" << std::endl;
        close(serverSocket);
        return 1;
    }
    std::cout << "bind successful" << std::endl;

    rc = listen(serverSocket, SOMAXCONN);
    if (rc < 0)
    {
        std::cerr << "listen()" << std::endl;
        close(serverSocket);
        return 1;
    }
    std::cout << "listen successful" << std::endl;

    for (;;)
    {
        std::cout << "waiting for connections..." << std::endl;

        int clientSocket = accept(serverSocket, nullptr, nullptr);
        std::cout << "got a connection" << std::endl;

        rc = handle_connection(clientSocket);
        if (rc < 0)
        {
            std::cerr << "handle connection failed" << std::endl;
        }
    }

    return 0;
}

std::string http_status_to_string(http_status_code status)
{
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

int handle_connection(int socket)
{
    char request_buffer[1024] = {0};
    ssize_t rc = 0;

    for (;;)
    {
        rc = recv(socket, request_buffer, sizeof(request_buffer), 0);
        if (rc < 0)
        {
            std::cerr << "recv()" << std::endl;
            close(socket);
            return -1;
        }
        request_buffer[rc] = '\0';

        http_request_line request_line;
        request_line.method = "";
        request_line.uri = "";
        request_line.version = "";

        std::string request(request_buffer);
        http_status_code status = parse_request(request, request_line);
        if (status != HTTP_RES_OK)
        {
            return -1;
        }

        std::string route_root = "/";
        std::string route_fn = "index.html";
        if (route_root == request_line.uri)
        {
            rc = serve_file(socket, route_fn);
            if (rc < 0)
            {
                std::cout << "serve file failed" << std::endl;
                return -1;
            }
        } else
        {
            rc = send_response_header(socket, generate_response_header(HTTP_RES_NOT_FOUND, NOT_FOUND_BODY.size()), NOT_FOUND_BODY);
            if (rc < 0)
            {
                std::cout << "send response header failed" << std::endl;
                return -1;
            }
            return -1;
        }
        close(socket);
        break;
    }

    return 0;
}

http_status_code parse_request(const std::string& request, http_request_line& request_line)
{
    if (request.empty())
    {
        std::cout << "parse request failed" << std::endl;
        return HTTP_RES_INTERNAL_SERVER_ERR;
    }

    size_t line_end = request.find(CRLF);
    if (line_end == std::string::npos)
    {
        std::cout << "bad request" << std::endl;
        return HTTP_RES_BAD_REQUEST;
    }

    std::string line = request.substr(0, line_end);

    size_t method_end = line.find(SP);
    size_t uri_end = line.find(SP, method_end + 1);

    if (method_end == std::string::npos || uri_end == std::string::npos)
    {
        std::cout << "bad request" << std::endl;
        return HTTP_RES_BAD_REQUEST;
    }

    request_line.method = line.substr(0, method_end);
    request_line.uri = line.substr(method_end + 1, uri_end - (method_end + 1));
    request_line.version = line.substr(uri_end + 1);

    return HTTP_RES_OK;
}

std::string generate_response_header(http_status_code status, size_t file_length)
{
    std::ostringstream response_header;
    response_header << "HTTP/1.1"
                    << SP
                    << status
                    << SP
                    << http_status_to_string(status)
                    << CRLF
                    << "Content-Length: "
                    << file_length
                    << CRLF
                    << CRLF;

    return response_header.str();

}

size_t send_response_header(int socket, const std::string& response_header, const std::string& response_header_body)
{
    size_t rc = send(socket, response_header.data(), response_header.size(), MSG_MORE);
    if (rc < 0)
    {
        std::cerr << "send()" << std::endl;
        return -1;
    }

    rc = send(socket, response_header_body.data(), response_header_body.size(), 0);
    if (rc < 0)
    {
        std::cerr << "send()" << std::endl;
        return -1;
    }
    return 0;
}

size_t serve_file(int socket, const std::string& filename)
{
    int in_fd = -1;
    std::filesystem::path file_path;
    std::string header;
    struct stat st;
    size_t sent = 0;
    off_t sendfile_offset = 0;
    ssize_t result = 0;

    std::ostringstream temp_path;
    temp_path << WEB_ROOT << filename;
    file_path = temp_path.str();

    std::cout << temp_path.str() << std::endl;

    if (stat(file_path.c_str(), &st) != 0)
    {
        send_response_header(socket, generate_response_header(HTTP_RES_NOT_FOUND, NOT_FOUND_BODY.size()), NOT_FOUND_BODY);
        std::cout << "file dosen't exist" << std::endl;
        if (in_fd != -1)
        {
            close(in_fd);
        }
        return -1;
    }

    size_t file_size = std::filesystem::file_size(file_path);

    header = generate_response_header(HTTP_RES_OK, file_size);
    ssize_t rc = send(socket, header.data(), header.size(), MSG_MORE);
    if (rc < 0)
    {
        std::cerr << "send()" << std::endl;
        if (in_fd != -1)
        {
            close(in_fd);
        }
        return -1;
    } else if (rc == 0)
    {
        std::cerr << "send() returned 0" << std::endl;
        if (in_fd != -1)
        {
            close(in_fd);
        }
        return -1;
    }

    in_fd = open(file_path.c_str(), O_RDONLY);
    if (in_fd < 0)
    {
        send_response_header(socket, generate_response_header(HTTP_RES_NOT_FOUND, NOT_FOUND_BODY.size()), NOT_FOUND_BODY);
        if (in_fd != -1)
        {
            close(in_fd);
        }
        return -1;
    }

    while (sent < file_size) {
        result = sendfile(socket, in_fd, &sendfile_offset, file_size);
        if (result < 0) {
            std::cerr << "sendfile()" << std::endl;
            send_response_header(socket, generate_response_header(HTTP_RES_NOT_FOUND, NOT_FOUND_BODY.size()), NOT_FOUND_BODY);
            if (in_fd != -1)
            {
                close(in_fd);
            }
            return -1;
        }
        sent += result;
    }

    if (in_fd != -1) {
        close(in_fd);
    }

    return 0;
}
