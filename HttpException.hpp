#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include <unistd.h>

#include <iostream>
#include <string>
#include <exception>
#include <fstream>

class HttpException : public std::exception {
	public:
		HttpException(int code, std::string file, std::string message)
		{
			this->code = code;
			this->file = file;
			this->message = message;
			this->body = extractFile();
		}
		virtual ~HttpException() throw () {}
		virtual const char * what () const throw ()
		{
			return message.c_str();
		}
		int getCode() const
		{
			return code;
		}
		std::string getBody() const
		{
			return body;
		}
	
	protected:
		int code;
		std::string file;
		std::string body;
		std::string message;
		std::string extractFile()
		{
			std::ifstream fileStream(this->file);
			std::string line;
			std::string html;

			while (std::getline(fileStream, line))
				html += line;
			return html;
		}
};

class BadRequestException : public HttpException {
	public:
		BadRequestException(std::string path) : HttpException(400, path, "Bad Request") {}
};

class ForbiddenException : public HttpException {
	public:
	ForbiddenException(std::string path) : HttpException(403, path, "Forbidden") {}
};

class NotFoundException : public HttpException {
	public:
	NotFoundException(std::string path) : HttpException(404, path, "Not Found") {}
};

class MethodNotAllowedException : public HttpException {
	public:
		MethodNotAllowedException(std::string path) : HttpException(405, path, "Method Not Allowed") {}
};

class ConflictException : public HttpException {
	public:
		ConflictException() : HttpException(409, "", "Conflict") {}
};

class PayLoadTooLargeException : public HttpException {
	public:
		PayLoadTooLargeException(std::string path) : HttpException(413, path, "Payload Too Large") {}
};

class InternalServerErrorException : public HttpException {
	public:
		InternalServerErrorException(std::string path) : HttpException(500, path, "Internal Server Error") {}
};

class ServiceUnavailabledException : public HttpException {
	public:
		ServiceUnavailabledException(std::string path) : HttpException(503, path, "Service Unavailabled") {}
};






#endif