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
	headers["Method"] = method;
	if (str.find("/") != std::string::npos) //
	{
		subStr = str.substr(str.find("/"), (str.find(" ", str.find("/"))) - (str.find("/")));
		headers["Request-target"] = subStr;
	}
	else
		headers["Request-target"] = "target not defined"; //error
	if (str.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = str.find("HTTP");
		protocol = str.substr(protocol_index_start, (str.find("\n", protocol_index_start) - 1) - protocol_index_start);
		headers["Protocol"] = protocol;
	}
	else
		headers["Protocol"] = "protocol not defined";
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
		headers[key] = value; //should i have all lowercase?
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

std::map<int, struct header>  ClientRequest::parseBody(const char *buffer, int size)
{
	std::string contentType = headers["Content-Type"];
	std::string line;
	std::string key;
	std::string value;
	std::vector<int> binaryDataIndex;
	struct header data;
	int indexBinary = 0;

	std::cout << "parsing body" << std::endl;
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		std::vector<int> boundariesIndexes;
		char *b;
		b = getBoundary(buffer);
		int blen = strlen(b);
		for (int i = 0; i < size; i++) 
		{
			int diff = ft_memcmp(buffer + i, b, blen);
			// printf("diff: %d")
			if (diff == 0) {
				boundariesIndexes.push_back(i);
				//printf("boundary index: %d\n", i);
			}
		}
		for (int i = 1; i < (int)boundariesIndexes.size() - 1; i++) //excluding first and last
		{
			std::istringstream streamHeaders(buffer + boundariesIndexes[i]);
			indexBinary = boundariesIndexes[i];
			while (getline(streamHeaders, line))
			{
				std::cout << "line: " << line << std::endl;
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
					sections[i] = data;
					std::cout << key << std::endl;
					std::cout << value << std::endl;
					std::cout << "section size: " << sections.size() << std::endl;
				}
			}
			binaryDataIndex.push_back(indexBinary);
			streamHeaders.clear();
			line.clear();
			key.clear();
			value.clear();
			data.myMap.clear();
			indexBinary = 0;
			//ft_memset(&data, '\0', sizeof(data));
		}
	}
	else
	{
		printf("take care of text\n");
	}
	return (sections);
}

std::map<int, struct header> ClientRequest::getBodySections(void)
{
	return sections;
}

std::map<std::string, std::string> ClientRequest::getHeaders(void)
{
	return headers;
}
void ClientRequest::parseRequestHttp(const char *str, int size)
{
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseFirstLine(inputString);
	getline(request, line); //skipping first line
	parseHeaders(request);
	//check for body
	std::map<std::string, std::string>::iterator it = headers.find("Content-Length");
	if (it != headers.end())
	{
		printf("before body parse\n");
		parseBody(str, size);
	}
}