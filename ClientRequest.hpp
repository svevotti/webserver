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

#define PORT "8080"

class ClientRequest : public std::map<std::string, std::string> {

public:
	// ServerResponse();
	// // ServerResponse();
	// // Server(int);
	// ~ServerResponse();
	// ServerResponse(const ServerResponse &);
	// void	operator=(const ServerResponse &);
	std::map<std::string, std::string>	parseRequestHttp(const char *);
	void parseFirstLine(std::string);
	void parseHeaders(std::istringstream&);
	// int		getServerPort();

private:
	std::map<std::string, std::string> requestParse;

};

#endif