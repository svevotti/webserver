#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

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
#include <map>
#include <cstdio>

#include "ServerParseRequest.hpp"
#include "ServerStatusCode.hpp"
#include "InfoServer.hpp"

class ServerResponse
{

public:
	// ServerResponse();
	// // ServerResponse();
	// // SocketServer(int);
	// ~ServerResponse();
	// ServerResponse(const ServerResponse &);
	// void	operator=(const ServerResponse &);
	std::string responseGetMethod(InfoServer, std::map<std::string, std::string>);
	std::string responsePostMethod(InfoServer, std::map<std::string, std::string>);
	// int		getSocketServerPort();

};

#endif