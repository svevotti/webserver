#include "ClientRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <sstream>
#include <cstdio>

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

void ClientRequest::parseRequestLine(std::string str)
{
	std::string method;
	std::string subStr;
	std::string requestTarget;
	std::string protocol;
	std::string queryString;
	std::string url;

	method = findMethod(str);
	requestLine["Method"] = method;
	if (str.find("/") != std::string::npos) //
	{
		subStr = str.substr(str.find("/"), (str.find(" ", str.find("/"))) - (str.find("/")));
		requestLine["Request-URI"] = subStr; //Request-URI: full url+query
		if (subStr.find("?") != std::string::npos) //query
		{
			url = subStr.substr(0, subStr.find("?"));
			requestLine["Url"] = url; //url : only url
			queryString = subStr.substr(subStr.find("?") + 1, subStr.length() - subStr.find("?"));
			requestLine["Query-string"] = queryString; //query-string only query
			std::string key;
			std::string value;
			int i = 0;
			while (i < (int)queryString.length())
			{
				while (queryString[i] != '=')
				{
					if (i ==  (int)queryString.length())
						break;
					key.append(1, queryString[i]);
					i++;
				}
				if (i ==  (int)queryString.length())
					break;
				i++;
				while (queryString[i] != '&')
				{
					if (i ==  (int)queryString.length())
						break;
					value.append(1, queryString[i]);
					i++;
				}
				query[key] = value;
				if (i == (int)queryString.length())
					break;
				key.clear();
				value.clear();
				i++;
			}
		}
	}
	else
		requestLine["Request-URI"] = "target not defined"; //error
	if (str.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = str.find("HTTP");
		protocol = str.substr(protocol_index_start, (str.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestLine["Protocol"] = protocol;
	}
	else
		requestLine["Protocol"] = "protocol not defined";
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
int ClientRequest::getTypeBody(void) const
{
	return typeBody;
}

void ClientRequest::parseBody(std::string buffer, int size, std::istringstream& str)
{
	std::string contentType = headers["Content-Type"];
	std::string line;
	std::string key;
	std::string value;
	struct section data;
	int indexBinary = 0;

	std::cout << "parsing body" << std::endl;
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		this->typeBody = MULTIPART;
		std::vector<int> boundariesIndexes;
		char *b;
		b = getBoundary(buffer.c_str());
		int blen = strlen(b);
		for (int i = 0; i < size; i++) 
		{
			int diff = ft_memcmp(buffer.c_str() + i, b, blen);
			if (diff == 0) {
				boundariesIndexes.push_back(i-2); //correct?
			}
		}
		std::string headers = buffer.substr(0, boundariesIndexes[1]);
		for (int i = 1; i < (int)boundariesIndexes.size() - 1; i++) //excluding first and last
		{
			std::istringstream streamHeaders(buffer.c_str() + boundariesIndexes[i]);
			indexBinary = boundariesIndexes[i];
			while (getline(streamHeaders, line))
			{
				if (line.find_first_not_of("\r\n") == std::string::npos)
					break ;
				indexBinary += line.length() + 1; //ADD \n in the count
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
				}
			}
			data.indexBinary = indexBinary+2;
			binaryIndex.push_back(indexBinary);
			data.body.append(buffer.c_str() + indexBinary+2, buffer.c_str() + boundariesIndexes[i+1]-2);
			sectionsVec.push_back(data);
			streamHeaders.clear();
			line.clear();
			key.clear();
			value.clear();
			data.myMap.clear();
			indexBinary = 0;
			data.body.clear();
		}
	}
	else
	{
		typeBody = TEXT;
		std::string line;
		getline(str,line);
		while (getline(str, line))
		{
			body.append(line);
		}
	}
}

std::map<int, struct section> ClientRequest::getBodySections(void) const
{
	return sections;
}

std::map<std::string, std::string> ClientRequest::getRequestLine(void) const
{
	return requestLine;
}

std::map<std::string, std::string> ClientRequest::getHeaders(void) const
{
	return headers;
}

std::map<std::string, std::string> ClientRequest::getQueryMap(void) const
{
	return query;
}

std::vector<int> ClientRequest::getBinaryIndex(void) const
{
	return binaryIndex;
}

std::string ClientRequest::getBodyText(void) const
{
	return body;
}

std::vector<struct section> ClientRequest::getSections(void) const
{
	return this->sectionsVec;
}
void ClientRequest::parseRequestHttp(std::string str, int size)
{
	std::string inputString(str);
	std::istringstream request(inputString);
	std::string line;
	std::string key;
	std::string value;

	parseRequestLine(inputString);
	std::cout << "parse headers" << std::endl;
	getline(request, line); //skipping first line
	parseHeaders(request);
	std::map<std::string, std::string>::iterator it = headers.find("Content-Length");
	if (it != headers.end())
	{
		printf("before body parse\n");
		parseBody(str, size, request);
	}
}