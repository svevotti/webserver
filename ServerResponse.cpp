#include "ServerResponse.hpp"
#include <fstream>
#include <dirent.h>

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
		if (requestTarget == "/") //what client wants - in this case we give back default page(index.html)
		{
			// locate file: open home dir, see the contents
			std::string directoryPath = "." + requestTarget; //only slash means current directory ?
			struct dirent *folder;
			std::string defaultPage;
			std::vector<std::string> listContentDirectory;
			std::string str;
			DIR *dir;
			
			dir = opendir(directoryPath.c_str());
			if (dir == NULL)
				std::cerr << "Error in opening directory" << std::endl;
			while ((folder = readdir(dir)) != NULL)
			{
				// std::cout << "content in folder: " << folder->d_name << std::endl;
				std::string str(folder->d_name);
				if (str == "index.html")
					defaultPage = str;
			}
			std::fstream file;
			std::string line;
			std::string bodyHtml;
			int bodyHtmlLen;

			// check if it exists
			file.open(defaultPage, std::fstream::in | std::fstream::out); //checking if i can open file, ergo it exists
			if (!file)
				std::cerr << "Error in opening file" << std::endl;
			// get file content into string
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
			//get body size
			bodyHtmlLen = bodyHtml.length();
			// assemle response
			response =	"HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: \r\n" //need to exactly the message's len, or it doesn't work
						"Connection: close\r\n" //client end connection right away, keep-alive
						"\r\n";
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