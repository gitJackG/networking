#include <string>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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

        // rc = handle_connection(clientSocket);
	}

    return 0;
}