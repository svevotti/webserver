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
ServerSockets::ServerSockets(InfoServer info)
{
	initSockets(info);
}

//setter and getters
std::vector<int>	ServerSockets::getServerSockets(void) const
{
	return this->_serverFds;
}

//main functions
void	ServerSockets::initSockets(InfoServer info)
{
	int serverNumber = (int)info.getArrayPorts().size();
	this->_serverFds.resize(serverNumber);
	for (int i = 0; i < serverNumber; i++)
	{
		//should be a try and catch?
		this->_serverFds[i] = createSocket(info.getArrayPorts()[i].c_str());
		if (this->_serverFds[i] < 0)
		{
			Logger::error("Failed to create socket on port " + info.getArrayPorts()[i] + ": " + std::string(strerror(errno)));
			this->_serverFds.pop_back();
			continue;
		}
		Logger::info("Socker on port " + info.getArrayPorts()[i] + ": successfully created");
	}
}

int ServerSockets::createSocket(const char* portNumber)
{
	int fd;
	struct addrinfo hints, *serverInfo;
	int error;
	int opt = 1;

	Utils::ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; //flag to set iPV6
	hints.ai_socktype = SOCK_STREAM; //type of socket, we need TCP, stream socket
	hints.ai_flags = AI_PASSIVE; //flag to set localhost as server address
	error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
	if (error == -1)
		Logger::error("Failed getaddrinfo: " + std::string(strerror(errno)));
	fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
	if (fd == -1)
		Logger::error("Failed socket: " + std::string(strerror(errno)));
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
	if (listen(fd, 5) == -1)
	{
		Logger::error("Failed listen socket: " + std::string(strerror(errno)));
		return (-1);
	}
	return (fd);
}