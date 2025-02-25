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

typedef struct header
{
	std::map<std::string, std::string> myMap;
} header;

class ClientRequest
{

public:

	void parseRequestHttp(const char *, int);
	void parseRequestLine(std::string);
	void parseHeaders(std::istringstream&);
	void parseBody(const char *,int, std::istringstream&);
	std::map<int, struct header> getBodySections();
	std::map<std::string, std::string> getHeaders();
	std::map<std::string, std::string> getRequestLine();
	std::map<std::string, std::string> getQueryMap();
	std::vector<int> getBinaryIndex();
	std::string getBodyText();
	int getTypeBody(void);

private:
	std::map<std::string, std::string> requestLine;
	std::map<std::string, std::string> query;
	std::map<std::string, std::string> headers;
	std::map<int, struct header> sections;
	std::vector<int> binaryIndex;
	std::string body;
	int typeBody;
};

#endif