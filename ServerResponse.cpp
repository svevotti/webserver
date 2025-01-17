#include "ServerResponse.hpp"

std::string ServerResponse::AnalyzeRequest(std::map<std::string, std::string> request)
{
	std::string response;

	if (request["method"] == "GET")
	{
		std::string requestTarget = request["request-target"];
		std::string protocol = request["protocol"];
		// printf("request target %s\n", requestTarget.c_str());
		// printf("access : %i\n", access(path.c_str(), F_OK != -1));
		printf("protocol %s\n", request["protocol"].c_str());
		if (protocol != "HTTP/1.1")
		{
			response =	"HTTP/1.1 505 HTTP Version not supported\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: 103\r\n" //need to exactly the message's len, or it doesn't work
						"\r\n"
						"<html>\r\n"
						"<header>\r\n"
						"<title>Http vers</title>\r\n"
						"</header>\r\n"
						"<body>\r\n"
						"<h1>Http wrong!</h1>\r\n"
						"</body>\r\n"
						"</html>\r\n";
			return (response);
		}
		if (requestTarget == "/") //to show OK for first page
		{
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
		else //adding pages that don't exist
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