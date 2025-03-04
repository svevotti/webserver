#ifndef SERVER_STATUS_CODE_H
#define SERVER_STATUS_CODE_H

#include "ServerSockets.hpp"
#include <map>


class ServerStatusCode {

	public:

		ServerStatusCode();
		std::string getStatusCode(int);
		void setStatusCode();

	private:

		std::map<int, std::string> statusCode;

    };

#endif