#include "ServerResponse.hpp"
#include <fstream>
#include <dirent.h>
#include <sstream>

std::string	getHtmlPage(std::string path, std::string target)
{
	struct dirent *folder;
	std::string page;
	std::string str;
	DIR *dir;
	dir = opendir(path.c_str());
	if (dir == NULL)
		std::cerr << "Error in opening directory" << std::endl;
	while ((folder = readdir(dir)) != NULL)
	{
		std::string str(folder->d_name);
		if (str == target)
			page = str;
	}
	closedir(dir);
	return (page);
}

std::string getFileContent(std::string fileName)
{
	std::fstream file;
	std::string line;
	std::string bodyHtml;

	file.open(fileName.c_str(), std::fstream::in | std::fstream::out); //checking if i can open the file, ergo it exists
	if (!file)
	{
		std::cerr << "Error in opening the file" << std::endl;
		return (bodyHtml);
	}
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
	return (bodyHtml);
}

std::string ServerResponse::responseGetMethod(InfoServer info, std::map<std::string, std::string> request)
{
	std::string response;
	std::string page;
	std::string bodyHtml;
	int bodyHtmlLen;

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
	page = getHtmlPage(info.getConfigFilePath(), request["request-target"]);
	if (!page.empty())
	{
		//get body size
		bodyHtml = getFileContent(page);
		if (bodyHtml.empty())
			return (response);
		bodyHtmlLen = bodyHtml.length();
		// assemle response
		ServerStatusCode status;
		// response = status.getStatusCode()[200];
		/*should I also assemple the first 4 lines? for sure status code*/
		response =	"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html\r\n"
					"Content-Length: \r\n" //need to exactly the message's len, or it doesn't work
					"Connection: keep-alive\r\n" //client end connection right away, keep-alive
					"\r\n";
		//convert from int to std::string
		std::ostringstream intermediatestream;
		intermediatestream << bodyHtmlLen;
		std::string lenStr = intermediatestream.str();

		//add len in the response
		size_t pos = 0;
		pos = response.find("Content-Length:");
		pos = response.find(" ", pos);
		response.insert(pos + 1, lenStr);
		response = response.append(bodyHtml);
	}
	return (response);
}
