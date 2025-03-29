#include "HttpException.hpp"

std::string HttpException::htmlRootPath;

HttpException::HttpException(int code, std::string message)
{
	this->code = code;
	this->file = htmlRootPath + "/" + Utils::toString(code) + "./html";
	this->message = message;
	this->body = extractFile();
}

void HttpException::setHtmlRootPath(std::string uri)
{ 
	htmlRootPath = uri;
}

int HttpException::getCode() const
{
	return code;
}

std::string HttpException::getBody() const
{
	return body;
}

const char *HttpException::what () const throw ()
{
	return message.c_str();
}

std::string HttpException::extractFile(void)
		{
			std::ifstream fileStream(this->file.c_str());
			std::string line;
			std::string html;

			while (std::getline(fileStream, line))
				html += line;
			return html;
		}