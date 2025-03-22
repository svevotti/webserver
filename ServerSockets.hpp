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

#include "HttpResponse.hpp"
#include "InfoServer.hpp"
#include "HttpRequest.hpp"


class ServerSockets {
	public:
		// ServerSockets(InfoServer);
		ServerSockets(std::string ip, std::string port);
		// std::vector<int>	getServerSockets() const;
		int	getServerFd(void) const;
		// void				initSockets(InfoServer);
		void				initSockets(void);
		int					createSocket(void);
		// int					createSocket(const char*);

	private:
		// std::vector<int>	_serverFds;
		int fd;
		std::string ip;
		std::string port;
};

#endif