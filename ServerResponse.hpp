#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

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
#include <map>
#include <cstdio>

#include "ClientRequest.hpp"
#include "InfoServer.hpp"
#include "PrintingFunctions.hpp"

class ServerResponse
{

public:
	ServerResponse(ClientRequest, InfoServer);
	ServerResponse &operator=(ServerResponse const &other);
	std::string responseGetMethod();
	std::string responsePostMethod();
	std::string responseDeleteMethod();
	std::string pageNotFound(void);
	std::string handleFilesUploads();
	std::string getFileContent(std::string path);
	std::string getContentType(std::string str);
	std::string GenerateHttpResponse(std::string length);
	std::string getFileType(std::map<std::string, std::string> headers);
	std::string getFileName(std::map<std::string, std::string> headers);
	int checkNameFile(std::string str, std::string path);
	std::string GenerateStatusCode(int code);
	void createMapStatusCode();
private:
	ClientRequest request;
	InfoServer    info;
	int			 statusCode;
	std::map<int, std::string> mapStatusCode;
	

};

#endif