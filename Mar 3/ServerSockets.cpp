#include "ServerSockets.hpp"

#include "ServerSockets.hpp"
#include <sstream>

ServerSockets::ServerSockets(InfoServer info) 
{
    initSockets(info);
}

void ServerSockets::initSockets(InfoServer info) 
{
    int serverNumber = info.getServerNumber();
    _serverFds.resize(serverNumber, -1); // Initialize with -1
    for (int i = 0; i < serverNumber; i++) 
    {
        int fd = createSocket(info.getArrayPorts()[i].c_str());
        if (fd < 0)
            std::cerr << "[ERROR] Failed to create socket for port " << info.getArrayPorts()[i] << std::endl;
        _serverFds[i] = fd;
    }
}

int ServerSockets::createSocket(const char* portNumber) {
    struct addrinfo hints, *serverInfo = 0;
    int opt = 1;
    int fd;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, portNumber, &hints, &serverInfo) != 0) {
        std::cerr << "[ERROR] getaddrinfo failed for port " << portNumber << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] getaddrinfo succeeded for port " << portNumber << std::endl;

    fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
    if (fd == -1) {
        freeaddrinfo(serverInfo);
        std::cerr << "[ERROR] Socket creation failed for port " << portNumber << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] Socket created: fd=" << fd << std::endl;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        freeaddrinfo(serverInfo);
        close(fd);
        std::cerr << "[ERROR] Setsockopt failed for port " << portNumber << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] Setsockopt succeeded" << std::endl;

    if (bind(fd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
        freeaddrinfo(serverInfo);
        close(fd);
        std::cerr << "[ERROR] Bind failed for port " << portNumber << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] Bind succeeded" << std::endl;

    if (listen(fd, 5) == -1) {
        freeaddrinfo(serverInfo);
        close(fd);
        std::cerr << "[ERROR] Listen failed for port " << portNumber << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] Listen succeeded" << std::endl;

    freeaddrinfo(serverInfo);
    std::cout << "[LOG] Server socket created on port " << portNumber << std::endl;
    return fd;
}

std::vector<int> ServerSockets::getServerSockets() const 
{
    return _serverFds; // Direct return, no need for manual copying
}