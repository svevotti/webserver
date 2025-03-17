#ifndef HTTT_RESPONSE_H
#define HTTT_RESPONSE_H

#include <ctime>

#include "ClientHandler.hpp"

class HttpResponse {
	public:
		HttpResponse(int, std::string);
		std::string composeRespone();
		std::string generateStatusLine(int);
		std::string generateHttpHeaders();
		std::string verifyType(std::string);
		std::string findTimeStamp();

	private:
		int			statusCode;
		std::string body;
		std::map<int, std::string> mapStatusCode;
		
};

#endif