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
		//void		setImageType(std::string str);
		//NEW: +1 line
		void		setHeader(std::string key, std::string value);
		std::string composeResponse();
		std::string generateStatusLine(int);
		std::string generateHttpHeaders();
		std::string findType(std::string);
		std::string findTimeStamp();
		int			findFileType(std::string str);
		//NEW: +2 lines
		std::string	getBody() const { return body; }
		std::string	getHeader(std::string key) const;

	private:
		int			statusCode;
		std::string body;
		std::string redirectedUrl;
		std::map<int, std::string> mapStatusCode;
		std::string extension;
		//NEW: +1 line
		std::map<std::string, std::string> cgi_headers;

};

#endif
