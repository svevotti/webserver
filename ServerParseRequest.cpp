#include "ServerParseRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

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
	std::string requestTarget;
	std::string protocol;

	method = findMethod(str);
	requestParse["Method"] = method;
	if (str.find("/") != std::string::npos) // "/" means server's root path
	{
		requestTarget = str.substr(method.size() + 1, (str.find(" ", method.size() + 1)) - (method.size() + 1));
		if (requestTarget == "/") //asking for index.html
		{
			requestTarget.clear();
			requestTarget = "/index.html";
		}
		// else {
		// 	// look if directory exists
		// 	// if not return 404
		// 	// if yes return index.html of that directory
		// }
		requestParse["Request-target"] = requestTarget.erase(0, 1);
	}
	else
		requestParse["request-target"] = "target not defined"; //error
	if (str.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = str.find("HTTP");
		protocol = str.substr(protocol_index_start, (str.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestParse["Protocol"] = protocol;
	}
	else
		requestParse["protocol"] = "protocol not defined";
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

std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(char *str)
{
	/*will use stream to extract information*/
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseFirstLine(inputString);
	//parsing headers
	getline(request, line); //skipping first line
	parseHeaders(request);
	getline(request, line);
	//parsing body based on content lenght?
	std::map<std::string, std::string>::iterator it = requestParse.find("Content-Length");
	if (it == requestParse.end())
		std::cout << "no body found" << std::endl;
	else //parse body
	{
		//check content type
		//text or multidata
		//if text can be store in one string
		//if multidata -> it has different info, so need a struct for each of boundary
	}

	// printing parse http request as map
	// std::map<std::string, std::string>::iterator element;
	// std::map<std::string, std::string>::iterator ite = requestParse.end();

	// for(element = requestParse.begin(); element != ite; element++)
	// {
	// 	std::cout << "parsed item\nkey: " << element->first << " - value: " <<element->second << std::endl;
	// }
	return(requestParse);
}