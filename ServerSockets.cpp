#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "ServerSockets.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>

#include "Logger.hpp"

//constructor and destructor
ServerSockets::ServerSockets(std::string ip, std::string port)
{
	this->ip = ip;
	this->port = port;
	initSockets();
}
//setter and getters
int	ServerSockets::getServerSocket(void) const
{
	return this->fd;
}

//main functions
void	ServerSockets::initSockets(void)
{
	this->fd = createSocket();
	if (this->fd < 0)
	{
		Logger::error("Failed to create socket on port " + port + ": " + std::string(strerror(errno)));
		return;
	}
	Logger::info("Socker on port " + port + ": successfully created");
}

int ServerSockets::createSocket(void)
{
	int fd;
	struct addrinfo hints, *serverInfo;
	int error;
	int opt = 1;

	Utils::ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; //flag to set iPV4 - want to also handle ipv6?
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(this->ip.c_str(), this->port.c_str(), &hints, &serverInfo);
	if (error < 0)
		Logger::error("Failed getaddrinfo: " + std::string(gai_strerror(errno)));
	fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
	if (fd == -1)
	{
		Logger::error("Failed socket: " + std::string(strerror(errno)));
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		Logger::error("Failed set socket: " + std::string(strerror(errno)));
		close(fd);
		return (-1);
	}
	if (bind(fd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
	{
		Logger::error("Failed bind socket: " + std::string(strerror(errno)));
		return (-1);
	}
	freeaddrinfo(serverInfo);
	if (listen(fd, 128) == -1) //128 is backlog for incoming connections. 
	{
		Logger::error("Failed listen socket: " + std::string(strerror(errno)));
		return (-1);
	}
	return (fd);
}