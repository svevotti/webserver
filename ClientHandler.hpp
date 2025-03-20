#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "StringManipulations.hpp"
#include "HttpException.hpp"
#include "InfoServer.hpp"

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
	std::string raw_data;
	int totbytes;
	int fd;
	HttpRequest request;
	std::string response;
	ClientHandler(int fd)
	{
		this->fd = fd;
		this->totbytes = 0;
	}

	int readData(int fd, std::string &str, int &bytes);
	// void setRequest(HttpRequest request)
	// {
	// 	this->request = request;
	// }
	// void setResponse(std::string response)
	// {
	// 	this->response = response;
	// }
	// int getFd(void) const
	// {
	// 	return this->fd;
	// }
	// HttpRequest getRequest(void) const
	// {
	// 	return this->request;
	// }
	// std::string getResponse(void) const
	// {
	// 	return this->response;
	// }
	
	private:
	// int fd;
	// HttpRequest request;
	// std::string response;
};

#endif