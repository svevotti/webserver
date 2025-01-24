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

std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(char *str)
{
	/*will use stream to extract information*/
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;
	std::string method;
	std::string protocol;
	std::string requestTarget; //request target - what clients want

	//parsing first line only
	method = findMethod(inputString);
	requestParse["method"] = method;
	std::size_t request_index_start = method.size() + 2;
	if (inputString.find(" ", request_index_start) != std::string::npos)
	{
		requestTarget = inputString.substr(method.size() + 1, (inputString.find(" ", method.size() + 1)) - (method.size() + 1));
		if (requestTarget == "/")
		{
			requestTarget.clear();
			requestTarget = "/index.html";
		}
		// else {
		// 	// look if directory exists
		// 	// if not return 4040
		// 	// if yes return index.html of that directory
		// }

		requestParse["request-target"] = requestTarget.erase(0, 1);

	}
	else
		requestParse["request-target"] = "target not defined";
	if (inputString.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = inputString.find("HTTP");
		protocol = inputString.substr(protocol_index_start, (inputString.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestParse["protocol"] = protocol;
	}
	else
		requestParse["protocol"] = "protocol not defined";

	//parsing headers
	getline(request, line); //skipping first line
	while (getline(request, line))
	{
		if (line.find_first_not_of("\r\n") == std::string::npos)
			break ;
		if (line.find(":") != std::string::npos)
			key = line.substr(0, line.find(":"));
		if (line.find(" ") != std::string::npos)
			value = line.substr(line.find(" ") + 1);
		requestParse[key] = value; //should i have all lowercase?
	}
	//after empty line to check if there is a body
	getline(request, line);
	std::string body = line;
	if (!line.empty())
	{
		requestParse["body"] = body;
		std::cout << "is here the body? " << body << std::endl;
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