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

// Constructor
HttpResponse::HttpResponse(int code, std::string str)
{
	this->statusCode = code;
	this->body = str;

	//2xx is for success
	this->mapStatusCode.insert(std::pair<int, std::string>(200, "200 OK"));
	this->mapStatusCode.insert(std::pair<int, std::string>(201, "201 Created"));
	this->mapStatusCode.insert(std::pair<int, std::string>(204, "204 No Content"));

	//3xx is for redirection
	this->mapStatusCode.insert(std::pair<int, std::string>(301, "301 Moved Permanently"));

	//4xxis for client error
	this->mapStatusCode.insert(std::pair<int, std::string>(400, "400 Bad Request"));
	this->mapStatusCode.insert(std::pair<int, std::string>(403, "403 Forbidden"));
	this->mapStatusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(405, "405 Method Not Allowed"));
	this->mapStatusCode.insert(std::pair<int, std::string>(409, "409 Conflict"));
	this->mapStatusCode.insert(std::pair<int, std::string>(413, "413 Payload Too Large"));
	this->mapStatusCode.insert(std::pair<int, std::string>(415, "415 Unsupported Media Type"));

	//5xx for server error
	this->mapStatusCode.insert(std::pair<int, std::string>(500, "500 Internal Server Error"));
	this->mapStatusCode.insert(std::pair<int, std::string>(501, "501 Not Implemented"));
	this->mapStatusCode.insert(std::pair<int, std::string>(503, "503 Service Unavailabled"));
	this->mapStatusCode.insert(std::pair<int, std::string>(504, "504 Gateaway Timeout"));
	this->mapStatusCode.insert(std::pair<int, std::string>(505, "505 HTTP Version Not Supported"));
}

//Setters and Getters
void HttpResponse::setUriLocation(std::string url)
{
	this->redirectedUrl = url;
}

// Main functions
// std::string HttpResponse::composeRespone(void)
// {
// 	std::string response;
// 	std::string statusLine;
// 	std::string headers;


// 	statusLine = generateStatusLine(this->statusCode);
// 	response += statusLine;
// 	headers = generateHttpHeaders();
// 	response += headers + "\r\n";
// 	response += this->body;
// 	return response;
// }

std::string HttpResponse::composeRespone(std::string str)
{
	std::string response;
	std::string statusLine;
	std::string headers;

	
	statusLine = generateStatusLine(this->statusCode);
	response += statusLine;
	if (!str.empty())
		headers = str + "\r\n";
	else
		headers = generateHttpHeaders();
	response += headers + "\r\n";
	response += this->body;
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
		type = findType(this->body);
		headers += "Content-Type: " + type + "\r\n";
	}
	if (this->statusCode == 301)
		headers += "Location: " + this->redirectedUrl + "\r\n";
	length = Utils::toString(this->body.size());
	headers += "Content-Length: " + length + "\r\n";
	timeStamp = findTimeStamp() + "\r\n";
	headers += "Date: " + timeStamp;
	headers += "Cache-Control: no-store\r\n";
	headers += "Connection: keep-alive\r\n";
	return headers;
}

int HttpResponse::findFileType(std::string str)
{
	if (str.size() >= 2 && (static_cast<unsigned char>(str[0]) == 0xFF &&
						static_cast<unsigned char>(str[1]) == 0xD8))
		return 0; // JPEG
	if (str.size() >= 8 && (static_cast<unsigned char>(str[0]) == 0x89 && 
						static_cast<unsigned char>(str[1]) == 0x50 && 
						static_cast<unsigned char>(str[2]) == 0x4E && 
						static_cast<unsigned char>(str[3]) == 0x47 &&
						static_cast<unsigned char>(str[4]) == 0x0D && 
						static_cast<unsigned char>(str[5]) == 0x0A && 
						static_cast<unsigned char>(str[6]) == 0x1A && 
						static_cast<unsigned char>(str[7]) == 0x0A))
		return 1; // PNG
	if (str.size() >= 4 && (static_cast<unsigned char>(str[0]) == 0x47 && 
						static_cast<unsigned char>(str[1]) == 0x49 && 
						static_cast<unsigned char>(str[2]) == 0x46 && 
						static_cast<unsigned char>(str[3]) == 0x38))
		return 2; // GIF
	if (str.size() >= 4 && (static_cast<unsigned char>(str[0]) == 0x00 && 
						static_cast<unsigned char>(str[1]) == 0x00 && 
						static_cast<unsigned char>(str[2]) == 0x01 && 
						static_cast<unsigned char>(str[3]) == 0x00))
		return 3; // ICO
	return 4;
}

std::string HttpResponse::findType(std::string str)
{
	int magicNumber;

	if (str.find("<html") != std::string::npos || str.find("<!DOCTYPE") != std::string::npos)
		return "text/html";
	else if (str[1] == '{')
		return "application/json";
	magicNumber = findFileType(str);
	switch (magicNumber)
	{
		case 0:
			return "image/jpg";
		case 1:
			return "image/png";
		case 2:
			return "image/gif";
		case 3:
			return "image/x-icon";
	}
	return "text/plain";
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
