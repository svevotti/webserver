#include "ServerStatusCode.hpp"

void ServerStatusCode::setStatusCode(void)
{
	statusCode.insert(std::pair(200, "HTTP/1.1 200 OK\r\n"));
	statusCode.insert(std::pair(404, "HTTP/1.1 404 Not Found\r\n"));
}

std::map<int, std::string> ServerStatusCode::getStatusCode(void)
{
	return statusCode;
}