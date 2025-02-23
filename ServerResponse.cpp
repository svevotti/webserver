#include "ServerResponse.hpp"
#include <fstream>
#include <dirent.h>
#include <sstream>
#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>

std::string pageNotFound(void)
{
    std::string str =   "HTTP/1.1 404 Not Found\r\n"
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
    return (str);
}

std::string getFileContent(std::string path)
{
	std::ifstream file;
	std::string line;
	std::string bodyHtml;
	std::string temp;

	file.open(path.c_str(), std::fstream::in | std::fstream::out); //checking if i can open the file, ergo it exists
	if (!file)
	{
		std::cerr << "Error in opening the file" << std::endl;
		return (bodyHtml);
	}
	// get file content into string
	while (std::getline(file, line))
	{
		if (line.size() == 0)
			continue;
		else
			bodyHtml = bodyHtml.append(line + "\r\n");
	}
	file.close();
	return (bodyHtml);
}
//TODO: review this function
std::string getContentType(std::string str)
{
	std::string	fileExtText[] = {"html", "css", "txt"};
	std::string fileExtApp[] = {"json", "javascript", "pdf"};
	std::string fileExtImg[] = {"png", "gif", "jpg"};
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

//TODO: review this function
std::string assembleHeaders(std::string protocol, std::string length)
{
	std::string statusLine;
	ServerStatusCode CodeNumber;
	std::string contentType;
	std::string fileType;
	std::string fileExtension;

	statusLine += protocol + " " + CodeNumber.getStatusCode(200) + "\r\n";
	// statusLine += "Content-Type: ";
	// if (fileName.find(".") != std::string::npos)
	// {
	// 	fileExtension = fileName.substr(fileName.find(".") + 1);
	// 	contentType = getContentType(fileExtension);
	// }
	// statusLine += contentType + "\r\n";
	statusLine += "Content-Length: " + length + "\r\n";
	statusLine += "Connection: keep-alive\r\n"; //client end connection right away, keep-alive
	statusLine += "\r\n";
	return statusLine;
}

std::string ServerResponse::responseGetMethod(InfoServer info, ClientRequest request)
{
	std::string response;
	std::string bodyHtml;
	int bodyHtmlLen;
	std::ostringstream intermediatestream;
	std::string strbodyHtmlLen;
	std::string headers;
	std::string documentRootPath;
	std::string pathToTarget;
	struct stat pathStat;
	std::map<std::string, std::string> httpRequestLine;

	response = pageNotFound();
	httpRequestLine = request.getRequestLine();
	if (!(httpRequestLine["Request-target"].empty()))
	{
		documentRootPath = info.getServerDocumentRoot();
		pathToTarget = documentRootPath + httpRequestLine["Request-target"];
		if (stat(pathToTarget.c_str(), &pathStat) != 0)
			std::cout << "Error using stat" << std::endl;
		if (S_ISDIR(pathStat.st_mode))
		{
			if (pathToTarget[pathToTarget.length()-1] == '/')
				pathToTarget += "index.html";
			else
			pathToTarget += "/index.html";
		}
		bodyHtml = getFileContent(pathToTarget);
		if (bodyHtml.empty())
			return (response);
		bodyHtmlLen = bodyHtml.length();
		intermediatestream << bodyHtmlLen;
		strbodyHtmlLen = intermediatestream.str();
		headers = assembleHeaders(httpRequestLine["Protocol"], strbodyHtmlLen);
		response.clear();
		response += headers + bodyHtml;
	}
	return (response);
}

std::string getFileType(ClientRequest request)
{
	std::map<int, struct header> sections = request.getBodySections();
	std::map<int, struct header>::iterator outerIt;
	std::map<std::string, std::string>::iterator innerIt;
	std::string type;
	for (outerIt = sections.begin(); outerIt != sections.end(); outerIt++)
	{
		struct header section = outerIt->second;
		for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
		{
			if (innerIt->first == "Content-Type")
			{
				type = innerIt->second;
			}
		}
	}
	return type;
}

std::string getFileName(ClientRequest request)
{
	std::map<int, struct header> sections = request.getBodySections();
	std::map<int, struct header>::iterator outerIt;
	std::map<std::string, std::string>::iterator innerIt;
	std::string name;

	for (outerIt = sections.begin(); outerIt != sections.end(); outerIt++)
	{
		struct header section = outerIt->second;
		for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
		{
			if (innerIt->first == "Content-Disposition")
			{
				if (innerIt->second.find("filename") != std::string::npos)
				{
					std::string fileNameField = innerIt->second.substr(innerIt->second.find("filename"));
					if (fileNameField.find('"') != std::string::npos)
					{
						int indexFirstQuote = fileNameField.find('"');
						int indexSecondQuote = 0;
						if (fileNameField.find('"', indexFirstQuote+1) != std::string::npos)
						{
							indexSecondQuote = fileNameField.find('"', indexFirstQuote+1);
						}
						name = fileNameField.substr(indexFirstQuote+1,indexSecondQuote - indexFirstQuote-1);
					}
				}
			}
		}
	}
	return name;
}

int checkNameFile(std::string str, std::string path)
{
	DIR *folder;
	struct dirent *data;

	folder = opendir(path.c_str());
	std::string convStr;
	if (folder == NULL)
		printf("error opening folder\n");
	while ((data = readdir(folder)))
	{
		convStr = data->d_name;
		if (convStr == str)
			return (1);
	}
	closedir(folder);
	return (0);
}

std::string handleFilesUploads(InfoServer info, ClientRequest request, const char *buffer, int size)
{
	struct header section;
	std::map<int, struct header> httpBody;
	std::map<std::string, std::string> httpRequestLine;
	std::vector<int> dataIndex;
	std::string response;

	if (httpBody.size() > 1)
	{
		response = "HTTP/1.1 400 Bad Request\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: 0\r\n"
								"Connection: close\r\n"
								"\r\n";
		return response;
	}
	httpRequestLine = request.getRequestLine();
	httpBody = request.getBodySections();
	dataIndex = request.getBinaryIndex();
	std::string requestTarget = httpRequestLine["Request-target"];
	requestTarget.erase(requestTarget.begin());
	std::string pathFile = info.getServerRootPath() + requestTarget; //it only works if given this path by the client?
	//openfile
	std::string fileName = getFileName(request);
	std::string fileType = getFileType(request);
	//check if file exists already
	if (checkNameFile(fileName, pathFile) != 0)
	{
		std::cout << "File with name already existing, please change it\n";
		response = "HTTP/1.1 400 Bad Request\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: 0\r\n"
								"Connection: close\r\n"
								"\r\n";
		return (response);
	}
	//std::cout << "file name: " << fileName << std::endl;
	pathFile += "/" + fileName;
	//std::cout << pathFile << std::endl;
	int file = open(pathFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0) {
		perror("Error opening file");
		exit(1);
	}
	//write to the file
	ssize_t written = write(file, buffer + dataIndex[0] + 2, size - dataIndex[0] - 2);
	if (written < 0) {
		perror("Error writing to file");
		close(file);
		exit(1);
	}
	close(file);
	response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 0\r\n"
				"Connection: keep-alive\r\n"
				"\r\n"; //very imporant
	return (response);
}

std::string ServerResponse::responsePostMethod(InfoServer info, ClientRequest request, const char *buffer, int size)
{
	std::string response;
	if (request.getTypeBody() == MULTIPART)
		response = handleFilesUploads(info, request, buffer, size);
	else
	{
		response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 0\r\n"
				"Connection: keep-alive\r\n"
				"\r\n"; //very imporan
		printf("do something with text body\n");
	}
	return (response);
}

std::string ServerResponse::responseDeleteMethod(InfoServer info, ClientRequest request)
{
	std::cout << "Delete method" << std::endl;

	std::string response = pageNotFound();
	//TODO: check path to resource, if it exits delete, if not send negative response
	std::string pathToResource = info.getServerRootPath() + request.getRequestLine()["Request-target"];
	std::ifstream file(pathToResource.c_str());
	if (!(file.good()))
		return response;
	remove(pathToResource.c_str());
	response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 0\r\n"
				"Connection: keep-alive\r\n"
				"\r\n"; //very imporant
	//TODO: routing table or database on server side to easier retrieve resources?
	return response;
}