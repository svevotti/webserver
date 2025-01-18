#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

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


#define PORT "8080"

class SocketServer {

public:
	SocketServer();
	// SocketServer(int);
	~SocketServer();
	SocketServer(const SocketServer &);
	void	operator=(const SocketServer &);
	void	startSocket(InfoServer);
	// int		getSocketServerPort();

private:
	int _socketFd;
	int _newSocketFd;
	int _portNumber;
	struct sockaddr_in _serverAddr;
	struct sockaddr_in _clientAddr;
	socklen_t _lenClient;
};

#endif