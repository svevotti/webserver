#include "ServerStatusCode.hpp"

ServerStatusCode::ServerStatusCode()
{
	statusCode.insert(std::pair<int, std::string>(200, "200 OK"));
	statusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
}

std::string ServerStatusCode::getStatusCode(int code)
{
	return statusCode[code];
}