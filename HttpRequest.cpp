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
	
	parseRequestLine(inputString);
	getline(request, line);
	parseHeaders(request);
    it = headers.find("content-length");
	if (it != headers.end())
		parseBody(getHttpRequestLine()["method"], this->str, this->size);
	else
		typeBody = EMPTY;
}

void HttpRequest::parseBody(std::string method, std::string buffer, int size)
{
	std::string contentType = headers["content-type"];
	struct section data;
	std::map<std::string, std::string>::iterator it;

	if (method == "POST")
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
	else
	{
		typeBody = EMPTY;
		throw BadRequestException();
	}
}

void HttpRequest::parseRequestLine(std::string str)
{
	std::string subStr;
	std::string method;
	std::string uri;
	std::string protocol;
	size_t index;

	index = str.find(" ");
	if (index != std::string::npos)
	{
		method = str.substr(0, index);
		this->requestLine["method"] = method;
	}
	else
		throw BadRequestException();
	str.erase(0, method.length()+1);
	index = str.find(" ");
	if (index != std::string::npos)
	{
		uri = str.substr(0, index);
		if (uri.find("?") != std::string::npos)
		{
			std::string newUri = uri;
			newUri = uri.substr(0, uri.find("?"));
			uri.clear();
			exractQuery(uri);
			uri = newUri;
		}
		this->requestLine["request-uri"] = uri;
	}
	else
		throw BadRequestException();
	str.erase(0, uri.length()+1);
	index = str.find("\r\n");
	if (index != std::string::npos)
	{
		protocol = str.substr(0, index);
		requestLine["protocol"] = protocol;
	}
	else
		throw BadRequestException();
}

void HttpRequest::parseHeaders(std::istringstream& str)
{
	std::string line;
	std::string key;
	std::string value;
	size_t index;

	while (getline(str, line))
	{
		// std::cout << "line: " << line << std::endl;
		key.clear();
		value.clear();
		if (line.find_first_not_of("\r\n") == std::string::npos)
			break ;
		index = line.find(":");
		if (index != std::string::npos)
		{
			key = line.substr(0, index);
			if (line[line.find(":") + 1] != ' ')
				throw BadRequestException();
			value = line.substr(index + 2);
		}
		else
			throw BadRequestException();
		std::transform(key.begin(), key.end(), key.begin(), Utils::toLowerChar);
		// std::cout << "key: " << key << std::endl;
		std::transform(value.begin(), value.end(), value.begin(), Utils::toLowerChar);
		// std::cout << "value: " << value << std::endl;
		this->headers[key] = value;
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
		int diff = Utils::ft_memcmp(buffer.c_str() + i, b, blen);
		if (diff == 0)
		{
			boundariesIndexes.push_back(i-2); //correct?
		}
	}
	for (int i = 1; i < (int)boundariesIndexes.size() - 1; i++) //excluding first and last
	{
		extractSections(buffer, boundariesIndexes, i, b);
	}
	delete b;
}

void	HttpRequest::extractSections(std::string buffer, std::vector<int> indeces, int i, std::string b)
{
	int indexBinary = 0;
	std::string line;
	std::string key;
	std::string value;
	struct section data;
	size_t index;

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
			index = line.find(":");
			if (index != std::string::npos)
			{
				key = line.substr(0, index);
				if (line[line.find(":") + 1] != ' ')
					throw BadRequestException();
				value = line.substr(index + 2);
			}
			else
				throw BadRequestException();
			std::transform(key.begin(), key.end(), key.begin(), Utils::toLowerChar);
			std::transform(value.begin(), value.end(), value.begin(), Utils::toLowerChar);
			data.myMap[key] = value;
		}
	}
	data.indexBinary = indexBinary+2;
	data.body.append(buffer.c_str() + data.indexBinary, buffer.c_str() + indeces[i+1]-2);
	//std::cout << data.body << std::endl;
	sectionsVec.push_back(data);
}

// Utils
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
	requestLine["url"] = url; //url : only url
	queryString = str.substr(str.find("?") + 1, str.length() - str.find("?"));
	requestLine["query-string"] = queryString; //query-string only query
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
	const char *result = strstr(buffer, "boundary");
	result += 9;
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

void	HttpRequest::cleanProperties(void)
{
	str.clear();
	size = 0;
	requestLine.clear();
	query.clear();
	headers.clear();
	std::vector<struct section>::iterator it;
	sectionsVec.clear();
	if (!(sectionsVec.empty()))
		sectionsVec.pop_back();
	typeBody = EMPTY;
}
