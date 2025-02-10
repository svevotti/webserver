#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>

#include "ServerParseRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerStatusCode.hpp"
#include "StringManipulations.hpp"
#include "PrintingFunctions.hpp"


#define PORT "8080"

class Server {

public:
	void	startServer(InfoServer);
	// int		getServerPort();

private:
	int _serverFds;
};

#endif