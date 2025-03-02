#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

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
#include "ClientRequest.hpp"

#define TEXT 0
#define MULTIPART 1

typedef struct section
{
	std::map<std::string, std::string> myMap;
	std::string body;
	int indexBinary;
} header;

class HttpRequest
{

public:
    void HttpParse(std::string, int);
	void parseRequestHttp();
	void parseRequestLine(std::string);
	void parseHeaders(std::istringstream&);
	void parseBody(std::string,int, std::istringstream&);
	std::string findMethod(std::string);

	std::map<std::string, std::string> getHttpHeaders() const;
	std::map<std::string, std::string> getHttpRequestLine() const;
	std::map<std::string, std::string> getHttpUriQueryMap() const;
	std::string getHttpBodyText() const;
	int getHttpTypeBody(void) const;
	std::vector<struct section> getHttpSections() const;

private:
    std::string str;
    int size;
	std::map<std::string, std::string> requestLine;
	std::map<std::string, std::string> query;
	std::map<std::string, std::string> headers;
	std::vector<struct section> sectionsVec;
	int typeBody;
};

#endif