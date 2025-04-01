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

class ClientHandler {
	public:
		ClientHandler(int fd, InfoServer &configInfo);
		int getFd(void) const;
		HttpRequest getRequest() const;
		std::string getResponse() const;
		double getTime() const;
		int readData(int fd, std::string &str, int &bytes);
		int manageRequest(void);
		int isCgi(std::string str);
		std::string prepareResponse(struct Route route);
		std::string                                    retrievePage(struct Route route);
		std::string                                       uploadFile(std::string path);
		std::string                                          deleteFile(std::string path);
		std::string extractContent(std::string path);
		int	retrieveResponse(void);
		void validateHttpHeaders(void);
		std::string findDirectory(std::string uri);
		std::string extraFileName(std::string uri);
	private:
	std::string raw_data;
	int totbytes;
	int fd;
	HttpRequest request;
	std::string response;
	InfoServer configInfo;
	double startingTime;
};

#endif