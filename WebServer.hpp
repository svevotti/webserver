#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "HttpResponse.hpp"
#include "ServerSockets.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"
#include "ClientHandler.hpp"
#include "Config.hpp"
//NEW: +3 lines
#include "CGIHandler.hpp"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#define DISCONNECTED 1
#define STATIC 2
#define CGI 3
#define READ 4
#define WRITE 5

class Webserver {
	public:
		Webserver(Config &file);
		~Webserver();
		int										startServer(void);
		void									addServerSocketsToPoll(int fd);
		int										fdIsServerSocket(int fd);
		int										fdIsCGI(int fd);
		void									dispatchEvents(void);
		void									createNewClient(int fd);
		int										processClient(int fd, int event);
		void									closeSockets(void);
		std::vector<ClientHandler>::iterator	retrieveClient(int fd);
		void									removeClient(std::vector<struct pollfd>::iterator it);
		void									checkTime(void);
		InfoServer*								matchFD( int fd );

	private:
		std::vector<InfoServer*>	configInfo;
		std::vector<struct pollfd>	poll_sets;
		int							serverFd;
		std::vector<ClientHandler>	clientsList;
		//NEW: +1 line
		CGIHandler					cgiHandler;
};
#endif
