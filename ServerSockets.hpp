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
		ServerSockets(std::string ip, std::string port);
		int	getServerSocket(void) const;
		void				initSockets(void);
		int					createSocket(void);

	private:
		int	fd;
		std::string ip;
		std::string port;
};

#endif