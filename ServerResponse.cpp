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

std::string getContentType(std::string str)
{
	std::string	fileExtText[] = {"html", "css", "txt"};
	std::string fileExtApp[] = {"json", "javascript", "pdf"};
	std::string fileExtImg[] = {"jpeg", "png", "gif"};
	std::string fileType;
	int i = 0;
	
	for(i = 0; i < 3; i++)
	{
		if (fileExtText[i] == str)
			fileType = "text/" + str;
		else if (fileExtApp[i] == str)
			fileType = "application/" + str;
		else if (fileExtImg[i] == str)
			fileType = "image/" + str;
	}
	return (fileType);
}

std::string assembleHeaders(std::string protocol, std::string fileName, std::string length)
{
	std::string statusLine;
	ServerStatusCode CodeNumber;
	std::string contentType;
	std::string fileType;
	std::string fileExtension;

	statusLine += protocol + " " + CodeNumber.getStatusCode(200) + "\r\n";
	statusLine += "Content-Type: ";
	if (fileName.find(".") != std::string::npos)
	{
		fileExtension = fileName.substr(fileName.find(".") + 1);
		contentType = getContentType(fileExtension);
	}
	statusLine += contentType + "\r\n";
	statusLine += "Content-Length: " + length + "\r\n";
	statusLine += "Connection: keep-alive\r\n"; //client end connection right away, keep-alive
	statusLine += "\r\n";
	return statusLine;
}

std::string ServerResponse::responseGetMethod(InfoServer info, std::map<std::string, std::string> request)
{
	std::string response;
	std::string page;
	std::string bodyHtml;
	int bodyHtmlLen;
	std::ostringstream intermediatestream;
	std::string strbodyHtmlLen;
	std::string headers;

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
	page = getHtmlPage(info.getServerRootPath(), request["Request-target"]); //to get rid of extension in url need to rewrite url in config file
	if (!page.empty())
	{
		bodyHtml = getFileContent(page);
		if (bodyHtml.empty())
			return (response); //error in opening file?
		//get body size
		bodyHtmlLen = bodyHtml.length();
		//convert from int to std::string
		intermediatestream << bodyHtmlLen;
		strbodyHtmlLen = intermediatestream.str();
		// assemble response
		headers = assembleHeaders(request["Protocol"], page, strbodyHtmlLen);
		response.clear();
		response += headers + bodyHtml;
		// std::cout << "server response\n" << response << std::endl;
	}
	//it means page is empty and i should throw some error code
	return (response);
}

std::string ServerResponse::responsePostMethod(InfoServer info, std::map<std::string, std::string> request)
{
	//we can fake i saved and store the image when it is already there
	std::string response =
					"HTTP/1.1 200 OK\r\n"
					"Content-Length: 0\r\n"
					"Connection: close\r\n"
					"\r\n"; //very imporant
	(void)info;
	(void)request;
	// std::cout << "what should look like POST response?\n";

	return (response);
}
