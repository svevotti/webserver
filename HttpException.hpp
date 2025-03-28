#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include "Server.hpp"
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

class InternalServerErrorException : public HttpException {
	public:
		InternalServerErrorException() : HttpException(500, "Internal Server Error") {}
};

class ServiceUnavailabledException : public HttpException {
	public:
		ServiceUnavailabledException() : HttpException(503, "Service Unavailabled") {}
};






#endif