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
	return ("ERROR");
}

std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(char *str)
{
	std::string inputString(str);

	//method - what client wants to do with target
	std::string method;
	method = findMethod(inputString);
	requestParse["method"] = method;

	//request target - what clients want
	std::string requestTarget;
	std::size_t request_index_start = method.size() + 2;
	if (inputString.find(" ", request_index_start) != std::string::npos)
	{
		requestTarget = inputString.substr(method.size() + 1, (inputString.find(" ", method.size() + 1)) - (method.size() + 1));
		if (requestTarget == "/")
		{
			requestTarget.clear();
			requestTarget = "index.html";
		}
		requestParse["request-target"] = requestTarget;

	}
	else
		requestParse["request-target"] = "target not defined";

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
	if (requestParse["method"] == "GET")
	{
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
	}
	else if (requestParse["method"] == "POST")
	{
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
		
		//content type
		std::string contentType;
		if (inputString.find("Content-Type") != std::string::npos)
		{
			std::size_t contentType_index = inputString.find(" ", inputString.find("Content-Type")) + 1;
			contentType = inputString.substr(contentType_index, (inputString.find("\n", contentType_index) - 1) - contentType_index);
			requestParse["content-type"] = contentType;
		}
		else
			requestParse["content-type"] = "content-type not defined";
		
		//content length
		std::string contentLen;
		if (inputString.find("Content-Length") != std::string::npos)
		{
			std::size_t contentLen_index = inputString.find(" ", inputString.find("Content-Length")) + 1;
			contentLen = inputString.substr(contentLen_index, (inputString.find("\n", contentLen_index) - 1) - contentLen_index);
			requestParse["content-length"] = contentLen;
		}
		else
			requestParse["content-length"] = "content-length not defined";
		
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
		
		//body
		int pos = 0;
		int index_empty_line = 0;
		int currentIndex = 0;
		std::cout << str << std::endl;
		std::string line;
		while (1)
		{
			// std::cout << "pos: " << pos << std::endl;
			// std::cout << "char at pos: " << inputString[pos] << std::endl;
			currentIndex = inputString.find("\n", pos);
			if (inputString.find("\n", pos) != std::string::npos)
			{
				// std::cout << "new line found at: " << currentIndex << std::endl;
				// std::cout << "current index + 1 should be next line: " << inputString[currentIndex + 1] << std::endl;
				line = inputString.substr(pos, currentIndex - pos);
				// std::cout << "getting substr for line : " << line << "size: " << line.size() << std::endl;
				if (line.size() == 1)
				{
					index_empty_line = currentIndex + 1;
					break ;
				}
			}
			line.clear();
			pos = currentIndex + 1;
		}
		std::string body = inputString.substr(index_empty_line);
		requestParse["body-post"] = body;
		// std::cout << "body: " << body << " end" << std::endl;
	}
	std::map<std::string, std::string>::iterator element;
	std::map<std::string, std::string>::iterator ite = requestParse.end();

	for(element = requestParse.begin(); element != ite; element++)
	{
		std::cout << "item: " << element->first << " - " <<element->second << std::endl;
	}
	return(requestParse);
}