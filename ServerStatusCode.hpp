#ifndef SERVER_STATUS_CODE_H
#define SERVER_STATUS_CODE_H

#include "SocketServer.hpp"


class ServerStatusCode
{
	public:
		std::map<int, std::string> getStatusCode();
		void setStatusCode();
	private:
		std::map<int, std::string> statusCode;
};




#endif