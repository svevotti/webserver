#include "ServerParseRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> splitStringByEmptyLines(std::string& inputString)
{
    std::vector<std::string> result;
  
    std::stringstream ss(inputString);
    std::string line;
	std::string new_line;

    // Loop until the end of the string
    while (getline(ss, line)) {
        if (!line.empty()) {
            result.push_back(line);
        }
    }

    return result;
}
char to_lowercase(unsigned char c)
{
    return std::tolower(c);
}

std::string findMethod(std::string inputStr)
{
	if (inputStr.find("GET") != std::string::npos)
		return ("GET");
	else if (inputStr.find("POST") != std::string::npos)
		return ("POST");
	else if (inputStr.find("DELETE") != std::string::npos)
		return ("DELETE");
	return ("ERROR");
}


//attention: so far, no body message in HTTP request - it is after all the HTTP headers
std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(char *str)
{
	std::string inputString(str);

	//method - what client wants to do with target
	std::string method;
	method = findMethod(inputString);
	requestParse["method"] = method;

	//target - what clients wants
	std::string requestTarget;
	std::size_t request_index_start = method.size() + 2;
	if (inputString.find(" ", request_index_start) != std::string::npos)
	{
		requestTarget = inputString.substr(method.size() + 1, (inputString.find(" ", method.size() + 1)) - (method.size() + 1));
		if (requestTarget == "/")
		{
			requestTarget.clear();
			requestTarget = "localhost";
		}
		requestParse["target"] = requestTarget;

	}
	else
		requestParse["target"] = "target not defined";

	//protocol - which protocol to send messages
	std::string protocol;
	if (inputString.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = inputString.find("HTTP");
		protocol = inputString.substr(protocol_index_start, (inputString.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestParse["protocol"] = protocol;
	}
	else
		requestParse["protocol"] = "protocol not defined";
	
	//these are all HTTP headers - additional information
	//accept
	std::string accept;
	if (inputString.find("Accept") != std::string::npos)
	{
		std::size_t accept_index_start = inputString.find(" ", inputString.find("Accept")) + 1;
		accept = inputString.substr(accept_index_start, (inputString.find("\n", accept_index_start) - 1) - accept_index_start);
		requestParse["accept"] = accept;
	}
	else
		requestParse["accept"] = "accept not defined";

	//user-agent
	std::string user_agent;
	if (inputString.find("User-Agent") != std::string::npos)
	{
		std::size_t user_agent_index_start = inputString.find(" ", inputString.find("User-Agent")) + 1;
		user_agent = inputString.substr(user_agent_index_start, (inputString.find("\n", user_agent_index_start) - 1) - user_agent_index_start);
		requestParse["user-agent"] = user_agent;
	}
	else
		requestParse["user-agent"] = "user agent not defined";
	
	//accept language
	std::string accept_language;
	if (inputString.find("Accept-Language") != std::string::npos)
	{
		std::size_t accept_language_index_start = inputString.find(" ", inputString.find("Accept-Language")) + 1;
		accept_language = inputString.substr(accept_language_index_start, (inputString.find("\n", accept_language_index_start) - 1) - accept_language_index_start);
		requestParse["accept-language"] = accept_language;
	}
	else
		requestParse["accept-language"] = "accept language not defined";
	
	//accept_encoding
	std::string accept_encoding;
	if (inputString.find("Accept_Encoding") != std::string::npos)
	{
		std::size_t accept_encoding_index_start = inputString.find(" ", inputString.find("Accept_Encoding")) + 1;
		accept_encoding = inputString.substr(accept_encoding_index_start, (inputString.find("\n", accept_encoding_index_start) - 1) - accept_encoding_index_start);
		requestParse["accept-encoding"] = accept_encoding;
	}
	else
		requestParse["accept-encoding"] = "accept-encoding not defined";

	//host
	std::string host;
	if (inputString.find("Host") != std::string::npos)
	{
		std::size_t host_index_start = inputString.find(" ", inputString.find("Host")) + 1;
		host = inputString.substr(host_index_start, (inputString.find("\n", host_index_start) - 1) - host_index_start);
		requestParse["host"] = host;
	}
	else
		requestParse["host"] = "host not defined";

	//referer
	std::string referer;
	if (inputString.find("Referer") != std::string::npos)
	{
		std::size_t referer_index_start = inputString.find(" ", inputString.find("Referer")) + 1;
		referer = inputString.substr(referer_index_start, (inputString.find("\n", referer_index_start) - 1) - referer_index_start);
		requestParse["referer"] = referer;
	}
	else
		requestParse["referer"] = "referer not defined";

	//authorization
	std::string authorization;
	if (inputString.find("Authorization") != std::string::npos)
	{
		std::size_t authorization_index_start = inputString.find(" ", inputString.find("Authorization")) + 1;
		authorization = inputString.substr(authorization_index_start, (inputString.find("\n", authorization_index_start) - 1) - authorization_index_start);
		requestParse["authorization"] = authorization;
	}
	else
		requestParse["authorization"] = "authorization not defined";

	// std::map<std::string, std::string>::iterator element;
	// std::map<std::string, std::string>::iterator ite = requestParse.end();

	// for(element = requestParse.begin(); element != ite; element++)
	// {
	// 	std::cout << "item: " << element->first << " - " <<element->second << std::endl;
	// }
	return(requestParse);
}