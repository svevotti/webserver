#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

#include "Utils.hpp"

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

class HttpResponse {
	public:
		HttpResponse(int, std::string);
		std::string composeRespone();
		std::string generateStatusLine(int);
		std::string generateHttpHeaders();
		std::string verifyType(std::string);
		std::string findTimeStamp();

	private:
		int			statusCode;
		std::string body;
		std::map<int, std::string> mapStatusCode;
		
};

#endif