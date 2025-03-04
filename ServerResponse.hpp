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
#include "ServerStatusCode.hpp"
#include "InfoServer.hpp"
#include "PrintingFunctions.hpp"

class ServerResponse
{

public:
	std::string responseGetMethod(InfoServer, ClientRequest);
	std::string responsePostMethod(InfoServer, ClientRequest);
	std::string responseDeleteMethod(InfoServer, ClientRequest);
	std::string pageNotFound(void);
	std::string handleFilesUploads(InfoServer info, ClientRequest request);
	std::string getFileContent(std::string path);
	std::string getContentType(std::string str);
	std::string assembleHeaders(std::string protocol, std::string length);
	std::string getFileType(std::map<std::string, std::string> headers);
	std::string getFileName(std::map<std::string, std::string> headers);
	int checkNameFile(std::string str, std::string path);

};

#endif