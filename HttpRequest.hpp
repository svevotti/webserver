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
#include <algorithm>
#include "Logger.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"

typedef struct section {
	std::map<std::string, std::string>	myMap;
	std::string 						body;
	int 								indexBinary;
}			   section;

class HttpRequest {
	public:
		HttpRequest() {};
		HttpRequest(HttpRequest const &other);

		std::map<std::string, std::string>	getHttpHeaders() const;
		std::map<std::string, std::string>	getHttpRequestLine() const;
		std::map<std::string, std::string>	getHttpUriQueryMap() const;
		struct section						getHttpSection() const;
		std::string 						getUri(void) const;
		std::string 						getMethod(void) const;
		std::string 						getQuery(void) const;
		std::string 						getBodyContent(void) const;
		std::string 						getHttpContentType (void) const;
		std::string 						getHttpContentLength(void) const;
		std::string 						getContentType (void) const;
		std::string 						getContentLength(void) const;
		// std::string							getPathInfo(void) const;
		// std::string							getRemoteAddr(void) const;
		// std::string							getRemoteHost(void) const;
		std::string 						getHost(void) const;
		std::string 						getProtocol(void) const;
		// std::string							getScriptName(void) const;
		// std::string							getServerPort(void)const;
		// std::string							getServerProtocol(void) const;
		std::string							findValue(std::map<std::string, std::string> map, std::string key) const;
		void								HttpParse(std::string, int);
		void								parseRequestHttp();
		void								parseRequestLine(std::string str);
		void 								setExtraHeaders(void);
		void								parseOtherTypes(std::string buffer);
		void 								unchunkData(void);
		void								exractQuery(std::string);
		std::string							decodeQuery(std::string str);
		void								parseHeaders(std::istringstream&);
		void								parseBody(std::string, std::string, int);
		void								parseMultiPartBody(std::string, int);
		char								*getBoundary(const char *);
		struct section						extractSections(std::string, int firstB, int secondB, std::string b);
		void								cleanProperties(void);

	private:
		int									size;
		std::string							str;
		std::map<std::string, std::string>	requestLine;
		std::map<std::string, std::string>	query;
		std::map<std::string, std::string>	headers;
		struct section						sectionInfo;
		std::string							body;
		std::string							content_type;
};

std::ostream &operator<<(std::ostream &output, HttpRequest const &request);

#endif