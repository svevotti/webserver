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

void ServerParseRequest::parseFirstLine(std::string str, std::string path)
{
	std::string method;
	std::string subStr;
	std::string requestTarget;
	std::string protocol;


	std::cout << "Str: " << str << std::endl;
	method = findMethod(str);
	requestParse["Method"] = method;
	if (str.find("/") != std::string::npos) // "/" means server's root path
	{
		subStr = str.substr(str.find("/"), (str.find(" ", str.find("/"))) - (str.find("/")));
		std::cout << "substring: " << subStr << "." << std::endl;
		if (subStr == "/" || subStr == "/index.html") //asking for index.html at root
			requestTarget = path + "index.html";
		else
		{
			// look if directory exists
			subStr.erase(0, 1);
			struct dirent *folder;
			std::string page;
			DIR *dir;

			// std::cout << "path - \n" << path << std::endl;
			// std::cout << "folder - \n" << subStr << std::endl;
			dir = opendir(path.c_str());
			if (dir == NULL)
				std::cerr << "Error in opening directory" << std::endl;
			while ((folder = readdir(dir)) != NULL)
			{
				// std::cout << "content: " << folder->d_name << std::endl;
				// std::cout << "folder: " << subStr << std::endl;
				std::string temp(folder->d_name);
				// std::cout << "temp: " << temp << std::endl;
				if ((temp + "/index.html")== subStr)
					requestTarget = path + subStr;
			}
			closedir(dir);
			// if not return 404
		}
	
		requestParse["Request-target"] = requestTarget;
		// std::cout << "path to index.html - " << requestTarget << std::endl;
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
		// std::cout << "line: " << line << std::endl;
		if (line.find_first_not_of("\r\n") == std::string::npos)
		{
			// std::cout << "empty line" << std::endl;
			break ;
		}
		if (line.find(":") != std::string::npos)
			key = line.substr(0, line.find(":"));
		if (line.find(" ") != std::string::npos)
			value = line.substr(line.find(" ") + 1);
		requestParse[key] = value; //should i have all lowercase?
		// std::cout << key << " - " << value << std::endl;
	}
}

std::map<std::string, std::string> ServerParseRequest::parseRequestHttp(const char *str, std::string path)
{
	/*will use stream to extract information*/
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseFirstLine(inputString, path);
	//parsing headers
	getline(request, line); //skipping first line
	parseHeaders(request);
	getline(request, line); //skip empty line
	//parsing body based on content lenght
	std::map<std::string, std::string>::iterator it = requestParse.find("Content-Length");
	if (it == requestParse.end())
		return (requestParse);
	// else //store body
	// {
	// 	if (inputString.find(line) != std::string::npos)
	// 	{
	// 		std::string body = inputString.substr(inputString.find(line));
	// 		requestParse["Body"] = body;
	// 	}
	// }
	// printing parse http request as map
	// std::map<std::string, std::string>::iterator element;
	// std::map<std::string, std::string>::iterator ite = requestParse.end();

	// for(element = requestParse.begin(); element != ite; element++)
	// {
	// 	std::cout << "parsed item\nkey: " << element->first << " - value: " <<element->second << std::endl;
	// }
	// std::cout << "before returning" << std::endl;
	return(requestParse);
}