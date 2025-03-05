#include "ClientRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <sstream>
#include <cstdio>

//constructor and destructor
ClientRequest::ClientRequest(void)
{
	
}
ClientRequest::ClientRequest(ClientRequest const &other)
{
	this->requestLine = other.requestLine;
	this->query = other.query;
	this->headers = other.headers;
	this->sectionsVec = other.sectionsVec;
	this->typeBody = other.typeBody;
}

ClientRequest &ClientRequest::operator=(ClientRequest const &other)
{
	if (this != &other)
	{
		this->requestLine = other.requestLine;
		this->query = other.query;
		this->headers = other.headers;
		this->sectionsVec = other.sectionsVec;
		this->typeBody = other.typeBody;
	}
	return *this;
}
//setter and getters
std::map<std::string, std::string> ClientRequest::getRequestLine(void) const
{
	return this->requestLine;
}

std::map<std::string, std::string> ClientRequest::getHeaders(void) const
{
	return this->headers;
}

std::map<std::string, std::string> ClientRequest::getUriQueryMap(void) const
{
	return this->query;
}

std::vector<struct section> ClientRequest::getSections(void) const
{
	return this->sectionsVec;
}

int ClientRequest::getTypeBody(void) const
{
	return this->typeBody;
}

std::map<std::string, std::string> ClientRequest::getSectionHeaders(int i) const
{
	return(this->sectionsVec[i].myMap);
}

std::string ClientRequest::getSectionBody(int i) const
{
	return(this->sectionsVec[i].body);
}

//main function
void ClientRequest::parseRequestHttp(std::string str, int size)
{
	HttpRequest request;
	
	request.HttpParse(str, size);
	this->requestLine = request.getHttpRequestLine();
	this->query = request.getHttpUriQueryMap();
	this->headers = request.getHttpHeaders();
	this->sectionsVec = request.getHttpSections();
	this->typeBody = request.getHttpTypeBody();
}