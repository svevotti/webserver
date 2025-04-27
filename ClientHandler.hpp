#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "HttpException.hpp"
#include "Config.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
//NEW: +2 lines
#include <sys/time.h>
#include <cstring>

#include <cstdio>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#define BUFFER 1024

//NEW: +2 lines
//Added by Simona, forward declaration for CGI
class Webserver;

class ClientHandler {
	public:
		ClientHandler(int fd, InfoServer const &configInfo);
		//NEW: +3 lines
		ClientHandler(const ClientHandler& other);
		ClientHandler& operator=(const ClientHandler& other);
		~ClientHandler() { Logger::debug("ClientHandler destructor for FD " + Utils::toString(fd)); }

		int 		getFd(void) const;
		double 		getTime(void) const;
		double 		getTimeOut(void) const;
		HttpRequest	getRequest(void) const;
		std::string getResponse(void) const;

		int 		manageRequest(void);
		int 		readData(int fd, std::string &str, int &bytes);
		std::string findDirectory(std::string uri);
		std::string createPath(struct Route route, std::string uri);
		void 		validateHttpHeaders(struct Route route);
		std::string	retrievePage(struct Route route);
		std::string extractContent(std::string path);
		std::string uploadFile(std::string path);
		std::string deleteFile(std::string path);
		std::string prepareResponse(struct Route route);
		int			retrieveResponse(void);
		int 		isCgi(std::string str);
		//NEW: +2 lines
		void 		setResponse(const std::string& resp);
		double		getCGIProcessingTimeout(void) const;

	private:
	int 		fd;
	int 		totbytes;
	std::string raw_data;
	double		startingTime;
	double		timeoutTime;
	InfoServer	configInfo;
	HttpRequest request;
	std::string response;
	//NEW: + 3 lines
	double		cgiProcessingTimeout;

	friend class	Webserver;
};

#endif
