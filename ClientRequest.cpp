#include "ClientRequest.hpp"
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

void ClientRequest::parseFirstLine(std::string str)
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

void ClientRequest::parseHeaders(std::istringstream& str)
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

char *getBoundary(const char *buffer)
{
	char *b;
	const char *result = strstr(buffer, "=");
	result++;
	int index_start = result - buffer;
	int count = 0;
	while (1)
	{
		if (*result == '\r')
			break ;
		count++;
		result++;
	}
	int i = 0;
	b = new char[count+1];
	while (i < count)
	{
		b[i] = buffer[index_start];
		i++;
		index_start++;
	}
	b[i] = '\0';
	return (b);
}

void ClientRequest::parseBody(const char *buffer)
{
	printf("buffer %s\n", buffer);
	std::map<std::string, std::string>::iterator it = requestParse.find("Content-Length");
	if (it->second == "0")
		return;
	std::istringstream oss(it->second);
	int size;
	oss >> size;
	std::vector<int> boundariesIndexes;
	//body parse
	std::string contentType = requestParse["Content-Type"];
	const char* red = "\033[31m";    // Red
    const char* green = "\033[32m";  // Green
    const char* yellow = "\033[33m"; // Yellow
    const char* blue = "\033[34m";   // Blue
    const char* reset = "\033[0m";
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		char *b;
		b = getBoundary(buffer);
		int blen = strlen(b);
		printf("len %d\n",blen);
		std::string boundary(b);
		for (int i = 0; i < size; i++) 
		{
			int diff = ft_memcmp(buffer + i, b, blen);
			if (diff == 0)
			{
				boundariesIndexes.push_back(i);
			}
		}
		//std::string boundary(b);
		std::cout << "boundary: " << boundary << std::endl;
		std::string line;
		struct header data;
		std::map<int, struct header> bodySections;
		std::string key;
		std::string value;
		//std::vector<int> binaryDataIndex;
		int indexBinary = 0;
		for (int i = 0; i < (int)boundariesIndexes.size(); i++)
		{
			std::cout << "index: " << boundariesIndexes[i] << std::endl;
			//std::cout << buffer + boundariesIndexes[i] << std::endl;
		}
		int flag = 0;
		for (int i = 1; i < (int)boundariesIndexes.size(); i++) //excluding first and last
		{
			//std::cout << "index: " << boundariesIndexes[i] << std::endl;
			std::istringstream streamHeaders(buffer + boundariesIndexes[i]);
			indexBinary = boundariesIndexes[i];
			while (getline(streamHeaders, line))
			{
				//std::cout << "line: " << line << std::endl;
				if (line == boundary + "--\r\n")
				{
					flag = 1;
					break;
				}
				if (line.find_first_not_of("\r\n") == std::string::npos)
					break ;
				indexBinary += line.length() + 1;
				if (line.find(b) != std::string::npos)
					continue;
				else
				{
					if (line.find(":") != std::string::npos)
						key = line.substr(0, line.find(":"));
					if (line.find(" ") != std::string::npos)
						value = line.substr(line.find(" ") + 1);
					data.myMap[key] = value;
				}
			}
			if (flag = 0)
				break ;
			//data.binaryDataIndex.push_back(indexBinary);
			data.binaryDataIndex = indexBinary;
			bodySections[i] = data;
			streamHeaders.clear();
			line.clear();
			key.clear();
			value.clear();
			indexBinary = 0;
		}
		std::map<int, struct header>::iterator outerIt;
		std::map<std::string, std::string>::iterator innerIt;
		for (outerIt = bodySections.begin(); outerIt != bodySections.end(); outerIt++)
		{
			//std::cout << "section: " << outerIt->first << std::endl;
			struct header section = outerIt->second;
			for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
			{
				//std::cout << innerIt->first << " : ";
				//std::cout << innerIt->second << std::endl;
			}
			//std::cout << outerIt->second.binaryDataIndex << std::endl;
		}
	}
}

void ClientRequest::parseRequestHttp(const char *str)
{
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseFirstLine(inputString);
	getline(request, line); //skipping first line
	parseHeaders(request);
	//getline(request, line); //skip empty line
	if (requestParse.find("Content-Length") != requestParse.end())
		parseBody(str);
	// return(requestParse);
}