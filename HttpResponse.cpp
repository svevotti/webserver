#include "HttpResponse.hpp"
#include <fstream>
#include <dirent.h>
#include <sstream>
#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>

// Constructor and destructor
HttpResponse::HttpResponse(int code, std::string str)
{
	this->statusCode = code;
	this->body = str;

	//2xx is for success
	this->mapStatusCode.insert(std::pair<int, std::string>(200, "200 OK"));
	this->mapStatusCode.insert(std::pair<int, std::string>(201, "201 Created"));
	this->mapStatusCode.insert(std::pair<int, std::string>(202, "202 Accepted"));
	this->mapStatusCode.insert(std::pair<int, std::string>(204, "204 No Content"));

	//3xx is for redirection
	this->mapStatusCode.insert(std::pair<int, std::string>(301, "301 Moved Permanently"));
	this->mapStatusCode.insert(std::pair<int, std::string>(302, "302 Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(304, "304 Not Modified"));

	//4xxis for client error
	this->mapStatusCode.insert(std::pair<int, std::string>(400, "400 Bad Request"));
	this->mapStatusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(405, "405 Method Not Allowed"));
	this->mapStatusCode.insert(std::pair<int, std::string>(409, "409 Conflict"));
	this->mapStatusCode.insert(std::pair<int, std::string>(413, "413 Payload Too Large"));

	//5xx for server error
	this->mapStatusCode.insert(std::pair<int, std::string>(500, "500 Internal Server Error"));
	this->mapStatusCode.insert(std::pair<int, std::string>(503, "503 Service Unavailabled"));
}

// Main functions
std::string HttpResponse::composeRespone(void)
{
	std::string response;
	std::string statusLine;
	std::string headers;

	statusLine = generateStatusLine(this->statusCode);
	response += statusLine;
	headers = generateHttpHeaders(); //dynamic assemble depending on method?
	response += headers + "\r\n";
	response += this->body;
	//Logger::debug("response: " + response);
	return response;
}

std::string HttpResponse::generateStatusLine(int code)
{
	std::string statusLine;
	std::map<int, std::string>::iterator it;

	it = this->mapStatusCode.find(code);
	if (it != this->mapStatusCode.end())
		return "HTTP/1.1 " + it->second + "\r\n";
	return "HTTP/1.1 500 Internal Server Error\r\n";
}

std::string HttpResponse::generateHttpHeaders(void)
{
	std::string headers;
	std::string type;
	std::string length;
	std::string	timeStamp;

	if (!(this->body.empty()))
	{
		type = verifyType(this->body); //now only html or jpeg
		headers += "Content-Type: " + type + "\r\n";
	}
	length = Utils::toString(this->body.size());
	headers += "Content-Length: " + length + "\r\n";
	timeStamp = findTimeStamp() + "\r\n";
	headers += "Date: " + timeStamp;
	headers += "Cache-Control: no-cache\r\n";
	headers += "Connection: keep-alive\r\n";
	return headers;
}

std::string HttpResponse::verifyType(std::string str)
{
	if (str.find("<html") != std::string::npos || str.find("<!DOCTYPE") != std::string::npos)
		return "text/html";
	return "image/jpeg";
}

std::string HttpResponse::findTimeStamp(void)
{
	std::time_t result = std::time(NULL);
	std::tm *localtime = std::localtime(&result);
	char *time = std::asctime(localtime);
	time[strlen(time) - 1] = '\0';
	std::string str(time);
	return str;
}

//NOTE: headers are case insensitive, order doesnt matter