#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

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
#include <vector>
#include "StringManipulations.hpp"
#include "HttpRequest.hpp"

#define TEXT 0
#define MULTIPART 1

class ClientRequest
{

public:
	void parseRequestHttp(std::string, int);
	
	std::map<std::string, std::string> getHeaders() const;
	std::map<std::string, std::string> getRequestLine() const;
	std::map<std::string, std::string> getUriQueryMap() const;
	std::string getBodyText() const;
	int getTypeBody(void) const;
	std::vector<struct section> getSections() const;
	std::map<std::string, std::string> getSectionHeaders(int) const;
	std::string getSectionBody(int) const;

private:
	std::map<std::string, std::string> requestLine;
	std::map<std::string, std::string> query;
	std::map<std::string, std::string> headers;
	std::vector<struct section> sectionsVec;
	int typeBody;
};

#endif