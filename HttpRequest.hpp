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
		std::vector<struct section>			getHttpSections() const;
		struct section						getHttpSection() const;
		std::map<std::string, std::string>	getSectionHeaders(int) const;
		std::string							getSectionBody(int) const;

		void								HttpParse(std::string, int);
		void								parseRequestHttp();
		void								parseRequestLine(std::string);
		void								parseHeaders(std::istringstream&);
		void								parseBody(std::string, std::string, int);
		void								exractQuery(std::string);
		void								parseMultiPartBody(std::string, int);
		// void								extractSections(std::string, std::vector<int>, int, std::string);
		struct section						extractSections(std::string, int firstB, int secondB, std::string b);
		char								*getBoundary(const char *);
		std::string							decodeQuery(std::string str);
		void								cleanProperties(void);
		void								validateParsedRequest(void);

	private:
		std::string							str;
		int									size;
		std::map<std::string, std::string>	requestLine;
		std::map<std::string, std::string>	query;
		std::map<std::string, std::string>	headers;
		// std::vector<struct section>			sectionsVec;
		struct section						sectionInfo;
};

#endif