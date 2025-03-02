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

#define TEXT 0
#define MULTIPART 1

typedef struct section
{
	std::map<std::string, std::string> myMap;
	std::string body;
	int indexBinary;
} header;

class ClientRequest
{

public:

	void parseRequestHttp(std::string, int);
	void parseRequestLine(std::string);
	void parseHeaders(std::istringstream&);
	void parseBody(std::string,int, std::istringstream&);

	std::map<std::string, std::string> getHttpHeaders() const;
	std::map<std::string, std::string> getHttpRequestLine() const;
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
	std::string body;
	std::vector<struct section> sectionsVec;
	int typeBody;
};

#endif