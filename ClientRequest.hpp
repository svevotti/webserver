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

typedef struct header
{
	std::map<std::string, std::string> myMap;
	int binaryDataIndex;
} header;

class ClientRequest
{

public:
	void parseRequestHttp(const char *);
	void parseFirstLine(std::string);
	void parseHeaders(std::istringstream&);
	void parseBody(const char *);
	std::map<std::string, std::string> getHeaders();
	struct header getBody();

private:
	std::map<std::string, std::string> requestParse;

};

#endif