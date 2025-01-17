#include "ServerResponse.hpp"

std::string ServerResponse::AnalyzeRequest(std::map<std::string, std::string> request)
{
	std::string response;

	if (request["method"] == "GET")
	{
		std::string path = request["target"];
		printf("path %s\n", path.c_str());
		// printf("access : %i\n", access(path.c_str(), F_OK != -1));
		if (path != "target not defined") //checkf if path is correct
		{
			std::cout << "here" << std::endl;
			response =	"HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: 103\r\n" //need to exactly the message's len, or it doesn't work
						"\r\n"
						"<html>\r\n"
						"<header>\r\n"
						"<title>Test page</title>\r\n"
						"</header>\r\n"
						"<body>\r\n"
						"<h1>Hello World</h1>\r\n"
						"</body>\r\n"
						"</html>\r\n";
		}
		else
		{
			// std::cout << "Not done yet" << std::endl;
			response =	"HTTP/1.1 404 Not Found\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: 103\r\n" //need to exactly the message's len, or it doesn't work
						"\r\n"
						"<html>\r\n"
						"<header>\r\n"
						"<title>Not Found</title>\r\n"
						"</header>\r\n"
						"<body>\r\n"
						"<h1>Not Found!!</h1>\r\n"
						"</body>\r\n"
						"</html>\r\n";
		}
		// printf("message to send assembled\n");
	}
	return (response);
}