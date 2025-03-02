#ifndef SERVER_SOCKETS_H
#define SERVER_SOCKETS_H

#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>

#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerStatusCode.hpp"
#include "StringManipulations.hpp"
#include "PrintingFunctions.hpp"
#include "HttpRequest.hpp"


class ServerSockets
{
public:
	ServerSockets(InfoServer);
	void initSockets(InfoServer);
	int createSocket(const char*);
	std::vector<int> getServerSockets() const;
private:
	std::vector<int> _serverFds;
};

#endif