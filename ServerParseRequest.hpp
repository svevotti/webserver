#ifndef SERVER_PARSE_REQUEST_H
#define SERVER_PARSE_REQUEST_H

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

#define PORT "8080"

class ServerParseRequest : public std::map<std::string, std::string> {

public:
	void parseRequestHttp(const char *);
	void parseFirstLine(std::string);
	void parseHeaders(std::istringstream&);
	std::map<std::string, std::string> GetHeaders(void);

private:
	std::map<std::string, std::string> requestParse;

};

#endif