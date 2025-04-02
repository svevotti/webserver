#include "HttpException.hpp"

std::string HttpException::htmlRootPath;

HttpException::HttpException(int code, std::string message)
{
	this->code = code;
	this->file = "." + htmlRootPath + "/" + Utils::toString(code) + ".html";
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
	std::ifstream inputFile(this->file.c_str(), std::ios::binary);

	if (!inputFile)
	{
		// std::cout << this->file << std::endl;
		std::cerr << "Error opening file httpexception" << std::endl;
		return "";
	}
	inputFile.seekg(0, std::ios::end);
	std::streamsize size = inputFile.tellg();
	inputFile.seekg(0, std::ios::beg);
	std::string html; 
	html.resize(size);
	if (!(inputFile.read(&html[0], size)))
		std::cerr << "Error reading file." << std::endl;
	inputFile.close();
	return html;
}