#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include "InfoServer.hpp"
#include "Utils.hpp"
#include <unistd.h>

#include <iostream>
#include <string>
#include <exception>
#include <fstream>

class HttpException : public std::exception {
	public:
		HttpException(int code, std::string message);
		virtual ~HttpException() throw () {}
		virtual const char * what () const throw ();
		int getCode(void) const;
		std::string getBody(void) const;
		static void setHtmlRootPath(std::string uri);

	protected:
		HttpException() {}
		int code;
		static std::string htmlRootPath;
		std::string file;
		std::string body;
		std::string message;
		std::string extractFile(void);
};

class BadRequestException : public HttpException {
	public:
		BadRequestException() : HttpException(400, "Bad Request") {}
};

class ForbiddenException : public HttpException {
	public:
		ForbiddenException() : HttpException(403, "Forbidden") {}
};

class NotFoundException : public HttpException {
	public:
		NotFoundException() : HttpException(404, "Not Found") {}
};

class MethodNotAllowedException : public HttpException {
	public:
		MethodNotAllowedException() : HttpException(405, "Method Not Allowed") {}
};

class ConflictException : public HttpException {
	public:
		ConflictException() : HttpException(409, "Conflict") {}
};

class PayLoadTooLargeException : public HttpException {
	public:
		PayLoadTooLargeException() : HttpException(413, "Payload Too Large") {}
};

class UnsupportedMediaTypeException : public HttpException {
	public:
		UnsupportedMediaTypeException() : HttpException(415, "Unsupported Media Type") {}
};

class InternalServerErrorException : public HttpException {
	public:
		InternalServerErrorException() : HttpException(500, "Internal Server Error") {}
};

class NotImplementedException : public HttpException {
	public:
		NotImplementedException() : HttpException(501, "Not Implemented") {}
};

class ServiceUnavailabledException : public HttpException {
	public:
		ServiceUnavailabledException() : HttpException(503, "Service Unavailabled") {}
};

class HttpVersionNotSupported : public HttpException {
	public:
		HttpVersionNotSupported() : HttpException(505, "Http Version Not Supported") {}
};






#endif