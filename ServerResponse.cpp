#include "ServerResponse.hpp"
#include <fstream>

std::string ServerResponse::AnalyzeRequest(std::map<std::string, std::string> request)
{
	//hardcoded
	// std::string homeDirectory = "./";
	std::string response;

	if (request["method"] == "GET")
	{
		std::string requestTarget = request["request-target"];
		std::string protocol = request["protocol"];
		// printf("request target %s\n", requestTarget.c_str());
		// printf("access : %i\n", access(path.c_str(), F_OK != -1));
		// printf("protocol %s\n", request["protocol"].c_str());
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
			// locate file: open home dir, see the contents
			// check if it exists
			// get file size (count characters)
			// get file content into string
			// assemle response
			std::fstream file;
			std::string line;
			std::string bodyHtml;
			int bodyHtmlLen;

			file.open("index.html", std::fstream::in | std::fstream::out);
			if (!file)
				std::cerr << "Error in opening file" << std::endl;
			while (std::getline(file, line))
			{
				// std::cout << "line: " << line << std::endl;
				if (line.size() == 0)
					continue;
				else
					bodyHtml = bodyHtml.append(line + "\r\n");
				// std::cout << "string: " << toString << std::endl;
			}
			file.close();
			response =	"HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: \r\n" //need to exactly the message's len, or it doesn't work
						"Connection: close\r\n" //client end connection right away, keep-alive
						"\r\n";
			bodyHtmlLen = bodyHtml.length();
			std::string lenStr = std::to_string(bodyHtmlLen);
			size_t pos = 0;
			pos = response.find("Content-Length:");
			pos = response.find(" ", pos);
			response.insert(pos + 1, lenStr);
			response = response.append(bodyHtml);
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