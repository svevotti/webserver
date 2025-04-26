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

#include <cstdio>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#define BUFFER 1024

class Webserver; // Added by Simona - Forward declaration for friend - needed for confinginfo-passing to CGI

class ClientHandler {
	public:
		ClientHandler(int fd, InfoServer const &configInfo);
		ClientHandler(const ClientHandler& other); // NEW - Simona - Copy constructor for debugging
		ClientHandler& operator=(const ClientHandler& other); // NEW - Simona - Copy assignment operator for debugging
		~ClientHandler() { Logger::debug("ClientHandler destructor for FD " + Utils::toString(fd)); } // NEW - Simona - Destructor

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
		void 		setResponse(const std::string& resp); // NEW - Simona - CGI integration
		double      getCGIProcessingTimeout(void) const; // NEW - Simona: for cgi_processing_timeout

	private:
		int 		fd;
		int 		totbytes;
		std::string raw_data;
		double		startingTime;
		double		timeoutTime;
		double      cgiProcessingTimeout; // NEW - Simona: cgi_processing_timeout
		InfoServer	configInfo;
		HttpRequest request;
		std::string response;
	
	friend class Webserver; // NEW - Simona - for confinginfo passing to CGI
};

#endif
