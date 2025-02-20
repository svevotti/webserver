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
} header;

class ClientRequest
{

public:

	void parseRequestHttp(const char *, int);
	void parseFirstLine(std::string);
	void parseHeaders(std::istringstream&);
	std::map<int, struct header>  parseBody(const char *,int);
	std::map<int, struct header> getBodySections();
	std::map<std::string, std::string> getHeaders();
	std::vector<int> getBinaryIndex();
	// int		getServerPort();

private:
	std::map<std::string, std::string> headers;
	std::map<int, struct header> sections;
	std::vector<int> binaryIndex;

};

#endif