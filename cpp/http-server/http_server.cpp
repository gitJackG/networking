#include <string>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define CRLF "\r\n"
#define SP " "

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

int handle_connection(int socket);
int handle_request(int socket);
http_status_code parse_request(const std::string& request, http_request_line& request_line);
std::string generate_response_header(std::string& request);
int handle_response(int socket, std::string& response_header);

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

int handle_connection(int socket)
{
    char request_buffer[1024] = {0};
    ssize_t rc = 0;

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
