#include "ServerResponse.hpp"
#include <fstream>
#include <dirent.h>




std::string ServerResponse::AnalyzeRequest(InfoServer info, std::map<std::string, std::string> request)
{
	//hardcoded
	// std::string homeDirectory = "./";
	std::string response;

	(void)info;
	if (request["method"] == "GET")
	{
		std::string requestTarget = request["request-target"];
		std::cout << "target: " << requestTarget << std::endl;
		if (requestTarget[0] == '/') //remove because already in the path
			requestTarget.erase(0, 1); 
		if (access(requestTarget.c_str(), F_OK) == 0) //it means is asking for default page, index.html
		{
			// locate file: open home dir, see the contents
			std::string directoryPath = info.getConfigFilePath();
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
				if (str == requestTarget)
					defaultPage = str;
			}
			closedir(dir);
			// check if it exists
			std::fstream file;
			std::string line;
			std::string bodyHtml;
			int bodyHtmlLen;
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
			ServerStatusCode status;
			// response = status.getStatusCode()[200];
			response =	"HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: \r\n" //need to exactly the message's len, or it doesn't work
						"Connection: keep-alive\r\n" //client end connection right away, keep-alive
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
	}
	return (response);
}