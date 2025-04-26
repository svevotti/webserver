#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

#include "Utils.hpp"
#include "HttpRequest.hpp"

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
		HttpResponse() {};
		HttpResponse(int code, std::string body);

		void 		setUriLocation(std::string url);
		void		setImageType(std::string str);
		void        setHeader(std::string key, std::string value); // NEW - Simona ( 4 CGI)
		std::string composeRespone();
		std::string generateStatusLine(int);
		std::string generateHttpHeaders();
		std::string findType(std::string);
		std::string findTimeStamp();

		std::string getBody() const { return body; } // NEW - Simona - CGI integration
		std::string getHeader(std::string key) const; // NEW - Simona - for CGI headers

	
	private:
		int			statusCode;
		std::string body;
		std::string redirectedUrl;
		std::map<int, std::string> mapStatusCode;
		std::string extension;
		std::map<std::string, std::string> cgi_headers; // NEW Simona - storage for custom CGI headers - CGI integration
		
};

#endif