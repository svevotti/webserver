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
		ClientHandler(int fd, InfoServer const &configInfo);
		int 		getFd(void) const;
		int 		getCGI_Fd(void) const;
		double 		getTime(void) const;
		double 		getTimeOut(void) const;
		int 		getPid(void) const;
		HttpRequest	getRequest(void) const;
		std::string getResponse(void) const;
		std::string getRawData() const {return raw_data;}
		int			checkRequestStatus(void);
		void		redirectClient(struct Route &route);
		void 		findPath(std::string str, struct Route &route);
		void		updateRoute(struct Route &route);
		int			readStdout(int fd);
		void		setResponse(std::string);
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
		int 		createResponse(void);
		int			retrieveResponse(void);
		int 		isCgi(std::string str);
		std::map<std::string, std::string>		parseScriptHeaders(void);

	private:
	int 		client_fd;
	int 		totbytes;
	std::string raw_data;
	double		startingTime;
	double		timeoutTime;
	InfoServer	configInfo;
	HttpRequest request;
	std::string response;
	int			internal_fd;
	int			pid;
};

#endif
