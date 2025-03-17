#include "HttpRequest.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <sstream>
#include <cstdio>

// Constructor and Destructor
HttpRequest::HttpRequest(HttpRequest const &other)
{
	this->requestLine = other.requestLine;
	this->query = other.query;
	this->headers = other.headers;
	this->sectionsVec = other.sectionsVec;
	this->typeBody = other.typeBody;
}
// Setters and getters
std::map<std::string, std::string> HttpRequest::getHttpRequestLine(void) const
{
	return requestLine;
}

std::map<std::string, std::string> HttpRequest::getHttpHeaders(void) const
{
	return headers;
}

std::map<std::string, std::string> HttpRequest::getHttpUriQueryMap(void) const
{
	return query;
}

std::vector<struct section> HttpRequest::getHttpSections(void) const
{
	return this->sectionsVec;
}

int HttpRequest::getHttpTypeBody(void) const
{
	return typeBody;
}

std::map<std::string, std::string> HttpRequest::getSectionHeaders(int i) const
{
	return(this->sectionsVec[i].myMap);
}

std::string HttpRequest::getSectionBody(int i) const
{
	return(this->sectionsVec[i].body);
}

// Main functions
void HttpRequest::HttpParse(std::string str, int size)
{
    this->str = str;
    this->size = size;
    parseRequestHttp();
}

void HttpRequest::parseRequestHttp(void)
{
	std::string inputString(this->str);
	std::istringstream request(inputString);
	std::string line;
    std::map<std::string, std::string>::iterator it;
	std::map<std::string, std::string>::iterator it1;

	parseRequestLine(inputString);
	getline(request, line);
	parseHeaders(request);
    it = headers.find("Content-Length");
	it1 = headers.find("Transfer-Encoding");
	if (it != headers.end() || it1 != headers.end())
		parseBody(getHttpRequestLine()["Method"], this->str, this->size);
	else
		typeBody = EMPTY;	
}

std::string HttpRequest::unchunkRequest(std::string chunked) 
{
	std::string::size_type bodyStart = chunked.find("\r\n\r\n");
	std::string newStr;
    std::string line;
    long chunk_size;
	std::string unchunked;

	if (bodyStart != std::string::npos)
	{
		bodyStart += 4;
		newStr = chunked.substr(bodyStart, chunked.length() - bodyStart - 2);
	}
	std::istringstream iss(newStr);
	while (std::getline(iss, line)) 
    {
        if (line == "\r")
			continue;
        char* end_ptr;
        chunk_size = strtol(line.c_str(), &end_ptr, 16);
		if (chunk_size == 0)
			break;
        if (*end_ptr != '\r' || chunk_size < 0)
			Logger::error("Invalid chunk size");
        std::vector<char> buffer(chunk_size);
        iss.read(&buffer[0], chunk_size);
        if (iss.gcount() != static_cast<std::streamsize>(chunk_size))
			Logger::error("Failed to read chunk data");
        unchunked.append(&buffer[0], chunk_size);
    }
    return unchunked;
}

void HttpRequest::parseBody(std::string method, std::string buffer, int size)
{
	std::string contentType = headers["Content-Type"];
	struct section data;
	std::map<std::string, std::string>::iterator it;

	if (method == "POST")
	{
		it = headers.find("Transfer-Encoding");
		if (it != headers.end())
		{
			this->typeBody = CHUNKED;
			data.indexBinary = 0;
			data.body = unchunkRequest(buffer);
			sectionsVec.push_back(data);
		}
		else
		{
			if (contentType.find("boundary") != std::string::npos) //multi format data
			{
				this->typeBody = MULTIPART;
				parseMultiPartBody(buffer, size);
			}
			else
			{
				struct section data;
				typeBody = TEXT;
				data.indexBinary = 0;
				std::string::size_type bodyStart = buffer.find("\r\n\r\n");
				if (bodyStart != std::string::npos)
				{
					bodyStart += 4;
					data.body = buffer.substr(bodyStart, buffer.length() - bodyStart - 2);
				}
				sectionsVec.push_back(data);	
			}
		}
	}
	else
		typeBody = EMPTY;
}

void HttpRequest::parseRequestLine(std::string str)
{
	std::string method;
	std::string subStr;
	std::string requestTarget;
	std::string protocol;
	std::string queryString;
	std::string url;

	//TODO:maybe split by spaces the first line
	method = findMethod(str);
	requestLine["Method"] = method;
	if (str.find("/") != std::string::npos)
	{
		subStr = str.substr(str.find("/"), (str.find(" ", str.find("/"))) - (str.find("/")));
		requestLine["Request-URI"] = subStr; //Request-URI: full url+query
		if (subStr.find("?") != std::string::npos) //query
			exractQuery(subStr);
	}
	else
		requestLine["Request-URI"] = "ERROR"; //error
	if (str.find("HTTP") != std::string::npos)
	{
		std::size_t protocol_index_start = str.find("HTTP");
		protocol = str.substr(protocol_index_start, (str.find("\n", protocol_index_start) - 1) - protocol_index_start);
		requestLine["Protocol"] = protocol;
	}
	else
		requestLine["Protocol"] = "ERROR";
}

void HttpRequest::parseHeaders(std::istringstream& str)
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
		headers[key] = value;
	}
}

void	HttpRequest::parseMultiPartBody(std::string buffer, int size)
{
	char *b;
	std::string line;
	std::string key;
	std::string value;
	std::vector<int> boundariesIndexes;

	b = getBoundary(buffer.c_str());
	int blen = strlen(b);
	for (int i = 0; i < size; i++)
	{
		int diff = ft_memcmp(buffer.c_str() + i, b, blen);
		if (diff == 0)
			boundariesIndexes.push_back(i-2); //correct?
	}
	for (int i = 1; i < (int)boundariesIndexes.size() - 1; i++) //excluding first and last
		extractSections(buffer, boundariesIndexes, i, b);
	delete b;
}

void	HttpRequest::extractSections(std::string buffer, std::vector<int> indeces, int i, std::string b)
{
	int indexBinary = 0;
	std::string line;
	std::string key;
	std::string value;
	struct section data;

	std::istringstream streamHeaders(buffer.c_str() + indeces[i]);

	indexBinary = indeces[i];
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
		}
	}
	data.indexBinary = indexBinary+2;
	data.body.append(buffer.c_str() + data.indexBinary, buffer.c_str() + indeces[i+1]-2);
	sectionsVec.push_back(data);
}

// Utils
std::string HttpRequest::findMethod(std::string inputStr)
{
	if (inputStr.find("GET") != std::string::npos)
		return ("GET");
	else if (inputStr.find("POST") != std::string::npos)
		return ("POST");
	else if (inputStr.find("DELETE") != std::string::npos)
		return ("DELETE");
	return ("ERROR");
}

std::string HttpRequest::decodeQuery(std::string str)
{
	std::string newStr;
	int start = 0;
	size_t pos = 0;
	std::string encode = "%20";
	std::string replace = " ";

	while (1)
	{
		pos = str.find(encode, start);
		if (pos == std::string::npos)
		{
			newStr += str.substr(start);
			break;
		}
		newStr += str.substr(start, pos - start);
		newStr += replace;
		start = pos + encode.length();
	}
	return newStr;
}

void HttpRequest::exractQuery(std::string str)
{
	std::string url;
	std::string queryString;
	std::string key;
	std::string value;
	size_t i = 0;

	url = str.substr(0, str.find("?"));
	requestLine["Url"] = url; //url : only url
	queryString = str.substr(str.find("?") + 1, str.length() - str.find("?"));
	requestLine["Query-string"] = queryString; //query-string only query
	while (i < queryString.length())
	{
		while (queryString[i] != '=')
		{
			if (i == queryString.length())
				break;
			key.append(1, queryString[i]);
			i++;
		}
		if (i == queryString.length())
			break;
		i++;
		while (queryString[i] != '&')
		{
			if (i == queryString.length())
				break;
			value.append(1, queryString[i]);
			i++;
		}
		query[key] = value;
		if (i == queryString.length())
			break;
		key.clear();
		value.clear();
		i++;
	}
}

char *HttpRequest::getBoundary(const char *buffer)
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
