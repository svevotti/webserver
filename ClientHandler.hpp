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
		// ClientHandler(int fd, InfoServer info);
		ClientHandler(int fd, InfoServer const &info);
		void storeInfoServer(void);
		int getFd(void) const;
		HttpRequest getRequest() const;
		std::string getResponse() const;
		int readData(int fd, std::string &str, int &bytes);
		int clientStatus(void);
		int isCgi(std::string str);
		// std::string prepareResponse(HttpRequest request);
		std::string prepareResponse(std::string localPath, std::string uri, std::string method);
		std::string                                    retrievePage(std::string localPath, std::string uri);
		std::string                                       uploadFile(std::string localPath, std::string uri);
		std::string                                          deleteFile(std::string localPath, std::string uri);
		int	retrieveResponse(void);

	private:
		std::string raw_data;
		int totbytes;
		int fd;
		HttpRequest request;
		std::string response;
		InfoServer info;

		std::string rootPath;
		std::string staticPath;
		std::string uploadPath;
		std::string errorPath;

	//InfoServer info;
};

#endif