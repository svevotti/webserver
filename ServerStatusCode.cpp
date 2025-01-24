#include "ServerStatusCode.hpp"

ServerStatusCode::ServerStatusCode()
{
	//2xx is for success
	statusCode.insert(std::pair<int, std::string>(200, "200 OK"));
	statusCode.insert(std::pair<int, std::string>(201, "201 Created"));
	statusCode.insert(std::pair<int, std::string>(204, "204 No Content"));

	//4xxis for client error
	statusCode.insert(std::pair<int, std::string>(400, "400 Bad Request"));
	statusCode.insert(std::pair<int, std::string>(401, "401 Unauthorized"));
	statusCode.insert(std::pair<int, std::string>(403, "403 Forbidden"));
	statusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
	statusCode.insert(std::pair<int, std::string>(409, "409 Conflict"));
}

std::string ServerStatusCode::getStatusCode(int code)
{
	return statusCode[code];
}