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
	this->sectionInfo = other.sectionInfo;
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

struct section HttpRequest::getHttpSection(void) const
{
	return this->sectionInfo;
}

std::string HttpRequest::findValue(std::map<std::string, std::string> map, std::string key) const
{
	return map[key];
}

std::string HttpRequest::getUri(void) const
{
	return findValue(this->requestLine, "request-uri");
}

std::string HttpRequest::getMethod(void) const
{
	return findValue(this->requestLine, "method");
}
std::string HttpRequest::getQuery(void) const
{
	return findValue(this->requestLine, "query-string");
}

std::string HttpRequest::getBodyContent(void) const
{
	return this->sectionInfo.body;
}

std::string HttpRequest::getContentType(void) const
{
	return findValue(this->sectionInfo.myMap, "content-type");
}

std::string HttpRequest::getContentLength(void) const
{
	return Utils::toString(this->sectionInfo.body.length());
}

std::string HttpRequest::getHost(void) const
{
	return findValue(this->headers, "host");
}

std::string HttpRequest::getProtocol(void) const
{
	return findValue(this->requestLine, "protocol");
}

std::string HttpRequest::getRawBody(void) const
{
	return this->raw_body;
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
		parseBody(requestLine["method"], this->str, this->size);
}

void HttpRequest::parseBody(std::string method, std::string buffer, int size)
{
	std::string contentType = headers["content-type"];
	struct section data;
	std::map<std::string, std::string>::iterator it;

	if (method == "POST")
	{	
		if (contentType.find("multipart/form-data") != std::string::npos)
			parseMultiPartBody(buffer, size);
		else
			throw NotImplementedException();
	}
	else
		throw BadRequestException();
}

void HttpRequest::parseRequestLine(std::string str)
{
	std::string subStr;
	std::string method;
	std::string uri;
	std::string protocol;
	size_t index;
	std::string newUri;

	//make also request line in lowercase
	index = str.find(" ");
	if (index != std::string::npos)
	{
		method = str.substr(0, index);
		std::transform(method.begin(), method.end(), method.begin(), Utils::toUpperCase);
		this->requestLine["method"] = method;
	}
	else
		throw BadRequestException();
	str.erase(0, method.length()+1);
	index = str.find(" ");
	if (index != std::string::npos)
	{
		uri = str.substr(0, index);
		newUri = uri;
		if (uri.find("?") != std::string::npos)
		{
			newUri = uri.substr(0, uri.find("?"));
			exractQuery(uri);
		}
		this->requestLine["request-uri"] = newUri;
	}
	else
		throw BadRequestException();
	str.erase(0, uri.length()+1);
	index = str.find("\r\n");
	if (index != std::string::npos)
	{
		protocol = str.substr(0, index);
		std::transform(method.begin(), method.end(), method.begin(), Utils::toUpperCase);
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
			size_t indexEnd = line.find("\r");
			if (indexEnd != std::string::npos)
				value = line.substr(index + 2, indexEnd - index - 2);
			else
				throw BadRequestException();
		}
		else
			throw BadRequestException();
		std::transform(key.begin(), key.end(), key.begin(), Utils::toLowerChar);
		std::transform(value.begin(), value.end(), value.begin(), Utils::toLowerChar);
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
	if (boundariesIndexes.size() > 3)
		throw NotImplementedException();
	int firstB = boundariesIndexes[1];
	int secondB = boundariesIndexes[2];
	this->raw_body = buffer.c_str() + firstB + blen + 4;
	this->sectionInfo = extractSections(buffer, firstB, secondB, b);
	delete b;
}

struct section HttpRequest::extractSections(std::string buffer, int firstB, int secondB, std::string b)
{
	int indexBinary = 0;
	std::string line;
	std::string key;
	std::string value;
	struct section data;
	size_t index;

	std::istringstream streamHeaders(buffer.c_str() + firstB);

	indexBinary = firstB;
	while (getline(streamHeaders, line))
	{
		if (line.find_first_not_of("\r\n") == std::string::npos)
			break ;
		indexBinary += line.length() + 1;
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
				size_t indexEnd = line.find("\r");
				if (indexEnd != std::string::npos)
					value = line.substr(index + 2, indexEnd - index - 2);
				else
					throw BadRequestException();
			}
			else
				throw BadRequestException();
			std::transform(key.begin(), key.end(), key.begin(), Utils::toLowerChar);
			std::transform(value.begin(), value.end(), value.begin(), Utils::toLowerChar);
			data.myMap[key] = value;
		}
	}
	data.indexBinary = indexBinary+2;
	data.body.append(buffer.c_str() + data.indexBinary, buffer.c_str() + secondB - 2);
	return data;
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
	requestLine["url"] = url;
	queryString = str.substr(str.find("?") + 1, str.length() - str.find("?"));
	std::string decodedQuery = decodeQuery(queryString);
	requestLine["query-string"] = decodedQuery;
	while (i < decodedQuery.length())
	{
		while (decodedQuery[i] != '=')
		{
			if (i == decodedQuery.length())
				break;
			key.append(1, decodedQuery[i]);
			i++;
		}
		if (i == decodedQuery.length())
			break;
		i++;
		while (decodedQuery[i] != '&')
		{
			if (i == decodedQuery.length())
				break;
			value.append(1, decodedQuery[i]);
			i++;
		}
		query[key] = value;
		if (i == decodedQuery.length())
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
	sectionInfo.body.clear();
	sectionInfo.indexBinary = 0;
	sectionInfo.myMap.clear();
}

std::ostream &operator<<(std::ostream &output, HttpRequest const &request) {
    // Print the request line
    std::map<std::string, std::string> requestLine = request.getHttpRequestLine();
    output << "HTTP Request Line:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = requestLine.begin(); it != requestLine.end(); ++it) {
        output << it->first << ": " << it->second << std::endl;
    }
    // Print the URI
    output << "URI: " << request.getUri() << std::endl;

    // Print the method
    output << "Method: " << request.getMethod() << std::endl;

    // Print the query
    output << "Query: " << request.getQuery() << std::endl;

    // Print the body content
    output << "Body Content: " << request.getBodyContent() << std::endl;

    // Print the content type
    output << "Content Type: " << request.getContentType() << ";" << std::endl;

    // Print the content length
    output << "Content Length: " << request.getContentLength() << std::endl;

    // Print the host
    output << "Host: " << request.getHost() << std::endl;

    // Print the protocol
    output << "Protocol: " << request.getProtocol() << std::endl;

	// Print the raw body
	output << "Raw Body: " << request.getRawBody() << std::endl;

    // Print the headers
    output << "Headers:" << std::endl;
    std::map<std::string, std::string> headers = request.getHttpHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        output << it->first << ": " << it->second << ";" << std::endl;
    }

    return output;
}