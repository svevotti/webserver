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

#include "PrintingFunctions.hpp"

#define SOCKET -1
#define GETADDRINFO -2
#define SETSOCKET -4
#define BIND -5
#define LISTEN -6

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
			printError(this->_serverFds[i]);
	}
}

int ServerSockets::createSocket(const char* portNumber)
{
	int fd;
	struct addrinfo hints, *serverInfo; //struct info about server address
	int error;
	int opt = 1;

	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; //flag to set iPV6
	hints.ai_socktype = SOCK_STREAM; //type of socket, we need TCP, stream socket
	hints.ai_flags = AI_PASSIVE; //flag to set localhost as server address
	error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
	if (error == -1)
		std::cout << "Error getaddrinfo" << std::endl;
	/*note: loop to check socket availabilyt or not*/
	fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0); //create socket
	if (fd == -1)
		printError(SOCKET);
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //make the address reusable
		printError(SETSOCKET);
	if (bind(fd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) //binding address to port number
		printError(BIND);
	/*till here the loop*/
	freeaddrinfo(serverInfo);
	if (listen(fd, 5) == -1) //make server socket listenning to incoming connections
		printError(LISTEN);
	return (fd);
}