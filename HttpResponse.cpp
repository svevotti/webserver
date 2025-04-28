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
	//NEW: +1 line (Easter egg)
	this->mapStatusCode.insert(std::pair<int, std::string>(418, "418 I'm a Teapot!"));

	//5xx for server error
	this->mapStatusCode.insert(std::pair<int, std::string>(500, "500 Internal Server Error"));
	this->mapStatusCode.insert(std::pair<int, std::string>(501, "501 Not Implemented"));
	//NEW: +1 line
	this->mapStatusCode.insert(std::pair<int, std::string>(502, "502 Bad Gateway"));
	this->mapStatusCode.insert(std::pair<int, std::string>(503, "503 Service Unavailable"));
	//NEW: +1 line
	this->mapStatusCode.insert(std::pair<int, std::string>(504, "505 Gateway Timeout"));
	this->mapStatusCode.insert(std::pair<int, std::string>(505, "505 HTTP Version Not Supported"));

}

//Setters and Getters
void HttpResponse::setUriLocation(std::string url)
{
	this->redirectedUrl = url;
}

// Deleted by SVEVA, correct?
// void HttpResponse::setImageType(std::string str)
// {
// 	this->extension = str;
// }

//NEW: Function
void HttpResponse::setHeader(std::string key, std::string value)
{
	this->cgi_headers[key] = value; // Store cgi header
}

// Main functions
std::string HttpResponse::composeResponse(void)
{
	std::string response;
	std::string statusLine;
	std::string headers;

	statusLine = generateStatusLine(this->statusCode);
	response += statusLine;
	headers = generateHttpHeaders();
	response += headers + "\r\n";
	//NEW: If loop, Sveva double check if this is redundant
	//Explanation: if chunked, don't append body here: formatting handled by caller (generateCGIResponse)
	if (cgi_headers.find("Transfer-Encoding") == cgi_headers.end() || cgi_headers["Transfer-Encoding"] != "chunked")
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

	//NEW: For loop, only debugging - remove?
	for (std::map<std::string, std::string>::iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it)
	{
		Logger::debug("CGI Header: " + it->first + ": " + it->second);
	}
	//NEW: +3 lines
	//Check for custom headers first, including Content-Type and Transfer-Encoding
	for (std::map<std::string, std::string>::iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it)
		headers += it->first + ": " + it->second + "\r\n";
	//NEW: first condition of the if loop
	if (cgi_headers.find("Content-Type") == cgi_headers.end() && !(this->body.empty()))
	{
		type = findType(this->body);
		headers += "Content-Type: " + type + "\r\n";
	}
	if (this->statusCode == 301)
		headers += "Location: " + this->redirectedUrl + "\r\n";
	//NEW: Included the next two lines in an if loop
	if (cgi_headers.find("Transfer-Encoding") == cgi_headers.end() && !this->body.empty())
	{
		length = Utils::toString(this->body.size());
		headers += "Content-Length: " + length + "\r\n";
	}
	timeStamp = findTimeStamp() + "\r\n";
	//NEW: Next three if loops, if the headers were not added by CGI yet, add them now
	if (cgi_headers.find("Date") == cgi_headers.end())
		headers += "Date: " + timeStamp;
	if (cgi_headers.find("Cache-Control") == cgi_headers.end())
		headers += "Cache-Control: no-store\r\n";
	if (cgi_headers.find("Connection") == cgi_headers.end())
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
	// if (str.size() >= 12 && (static_cast<unsigned char>(str[0]) == 0x52 &&
	// 					 static_cast<unsigned char>(str[1]) == 0x49 &&
	// 					 static_cast<unsigned char>(str[2]) == 0x46 &&
	// 					 static_cast<unsigned char>(str[3]) == 0x46 &&
	// 					 static_cast<unsigned char>(str[8]) == 0x57 &&
	// 					 static_cast<unsigned char>(str[9]) == 0x45 &&
	// 					 static_cast<unsigned char>(str[10]) == 0x42 &&
	// 					 static_cast<unsigned char>(str[11]) == 0x50))
	// 	return 3; // WEMP
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
