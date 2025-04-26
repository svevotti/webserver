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
	this->mapStatusCode.insert(std::pair<int, std::string>(204, "204 No Content"));

	//3xx is for redirection
	this->mapStatusCode.insert(std::pair<int, std::string>(301, "301 Moved Permanently"));

	//4xxis for client error
	this->mapStatusCode.insert(std::pair<int, std::string>(400, "400 Bad Request"));
	this->mapStatusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(405, "405 Method Not Allowed"));
	this->mapStatusCode.insert(std::pair<int, std::string>(409, "409 Conflict"));
	this->mapStatusCode.insert(std::pair<int, std::string>(413, "413 Payload Too Large"));
	this->mapStatusCode.insert(std::pair<int, std::string>(415, "415 Unsupported Media Type"));
	this->mapStatusCode.insert(std::pair<int, std::string>(418, "418 I'm a Teapot!")); // Simona's thing - added for fun

	//5xx for server error
	this->mapStatusCode.insert(std::pair<int, std::string>(500, "500 Internal Server Error"));
	this->mapStatusCode.insert(std::pair<int, std::string>(501, "501 Not Implemented"));
	this->mapStatusCode.insert(std::pair<int, std::string>(502, "502 Bad Gateway")); // added by Simona
	this->mapStatusCode.insert(std::pair<int, std::string>(503, "503 Service Unavailabled"));
	this->mapStatusCode.insert(std::pair<int, std::string>(504, "504 Gateaway Timeout")); // added by Simona
	this->mapStatusCode.insert(std::pair<int, std::string>(505, "HTTP Version Not Supported")); 
}

//Setters and Getters
void HttpResponse::setUriLocation(std::string url)
{
	this->redirectedUrl = url;
}

void HttpResponse::setImageType(std::string str)
{
	this->extension = str;
}

// START OF BIT BY SIMONA
void HttpResponse::setHeader(std::string key, std::string value)
{
    this->cgi_headers[key] = value; // Store cgi header
}
// END of IT

// Main functions
std::string HttpResponse::composeRespone(void) // Simona: typo?  Response
{
	std::string response;
	std::string statusLine;
	std::string headers;

	statusLine = generateStatusLine(this->statusCode);
	response += statusLine;
	headers = generateHttpHeaders();
	response += headers + "\r\n";
	
	//  SIMONA - added if-clause - CGI INTEGRATION 
	// Explanation: If chunked, don't append body here: formatting handled by caller (generateCGIResponse)
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

	// CHANGES BY SIMONA - CGI integration 
	// Log cgi_headers for debugging
	for (std::map<std::string, std::string>::iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it) 
	{
		Logger::debug("CGI Header: " + it->first + ": " + it->second);
	}

    // Check for custom headers first, including Content-Type and Transfer-Encoding
	for (std::map<std::string, std::string>::iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it) 
	{
		headers += it->first + ": " + it->second + "\r\n";
	}

	// Only set Content-Type if not already set by CGI
	if (cgi_headers.find("Content-Type") == cgi_headers.end() && !(this->body.empty()))
	{	
		type = findType(this->body);
		headers += "Content-Type: " + type + "\r\n";
	}
	if (this->statusCode == 301)
		headers += "Location: " + this->redirectedUrl + "\r\n";

	// IF-clause added by Simona
	// Explanation: Only add Content-Length if Transfer-Encoding: chunked is not set
	if (cgi_headers.find("Transfer-Encoding") == cgi_headers.end() && !this->body.empty())
	{
		length = Utils::toString(this->body.size());
		headers += "Content-Length: " + length + "\r\n";
	}
	timeStamp = findTimeStamp() + "\r\n";

	// Simona added if clauses
	// Explanation: Add standard headers if not set by CGI
	if (cgi_headers.find("Date") == cgi_headers.end())
		headers += "Date: " + timeStamp;
	if (cgi_headers.find("Cache-Control") == cgi_headers.end())
		headers += "Cache-Control: no-cache\r\n";
	if (cgi_headers.find("Connection") == cgi_headers.end())
		headers += "Connection: keep-alive\r\n";
	
	return headers;
}

std::string HttpResponse::findType(std::string str)
{
	int i = 0;
	if (str.find("<html") != std::string::npos || str.find("<!DOCTYPE") != std::string::npos)
		return "text/html";
	std::string	extensions[7] = {"jpg", "jpeg", "png", "gif", "bmp", "svg", "webp"};
	for (;i < 7; i++)
	{
		if (extensions[i] == this->extension)
			break;
	}
	switch (i)
	{
		case 0:
			return "image/jpg";
		case 1:
			return "image/jpeg";
		case 2:
			return "image/png";
		case 3:
			return "image/gif";
		case 4:
			return "image/bmp";
		case 5:
			return "image/svg";
		case 6:
			return "image/webp";
		default:
			return "image/x-icon";
	}
	return "";
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
