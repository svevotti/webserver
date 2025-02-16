#include "ServerParseRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <sstream>

std::string findMethod(std::string inputStr)
{
	if (inputStr.find("GET") != std::string::npos)
		return ("GET");
	else if (inputStr.find("POST") != std::string::npos)
		return ("POST");
	else if (inputStr.find("DELETE") != std::string::npos)
		return ("DELETE");
	return ("OTHER");
}

void ServerParseRequest::parseFirstLine(std::string str)
{
	std::string method;
	std::string subStr;
	std::string requestTarget;
	std::string protocol;


	method = findMethod(str);
	requestParse["Method"] = method;
	if (str.find("/") != std::string::npos) //
	{
		subStr = str.substr(str.find("/"), (str.find(" ", str.find("/"))) - (str.find("/")));
		requestParse["Request-target"] = subStr;
	}
	else
		requestParse["Request-target"] = "target not defined"; //error
	if (str.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = str.find("HTTP");
		protocol = str.substr(protocol_index_start, (str.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestParse["Protocol"] = protocol;
	}
	else
		requestParse["Protocol"] = "protocol not defined";
}

void ServerParseRequest::parseHeaders(std::istringstream& str)
{
	std::string line;
	std::string key;
	std::string value;
	while (getline(str, line))
	{
		if (line.find_first_not_of("\r\n") == std::string::npos)
			break ;
		if (line.find(":") != std::string::npos)
			key = line.substr(0, line.find(":"));
		if (line.find(" ") != std::string::npos)
			value = line.substr(line.find(" ") + 1);
		requestParse[key] = value; //should i have all lowercase?
	}
}

std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(const char *str)
{
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseFirstLine(inputString);
	getline(request, line); //skipping first line
	parseHeaders(request);
	getline(request, line); //skip empty line
	return(requestParse);
}