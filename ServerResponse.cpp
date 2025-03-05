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

//constructor and destructor
ServerResponse::ServerResponse(ClientRequest request, InfoServer info)
{
	createMapStatusCode();
	this->request = ClientRequest(request);
	this->info = InfoServer(info);
	this->statusCode = 200;
}

//setters and getters

//main functions
std::string ServerResponse::responseGetMethod()
{
	std::string response;
	std::string htmlFile;
	int bodyHtmlLen;
	std::ostringstream intermediatestream;
	std::string strbodyHtmlLen;
	std::string httpHeaders;
	std::string statusCodeLine;
	std::string documentRootPath;
	std::string pathToTarget;
	struct stat pathStat;
	std::map<std::string, std::string> httpRequestLine;

	//TODO: i think the 404 page can be hardcode it
	response = pageNotFound();
	httpRequestLine = request.getRequestLine();
		//create path to the index.html
	documentRootPath = info.getServerDocumentRoot();
	pathToTarget = documentRootPath + httpRequestLine["Request-URI"];
	if (stat(pathToTarget.c_str(), &pathStat) != 0)
		Logger::error("Failed stat: " + std::string(strerror(errno)));
	if (S_ISDIR(pathStat.st_mode))
	{
		if (pathToTarget[pathToTarget.length()-1] == '/')
			pathToTarget += "index.html";
		else
		pathToTarget += "/index.html";
	}
	//check if file exists
	htmlFile = getFileContent(pathToTarget);
	if (htmlFile.empty())
		return (response);
	bodyHtmlLen = htmlFile.length();
	intermediatestream << bodyHtmlLen;
	strbodyHtmlLen = intermediatestream.str();

	statusCodeLine = GenerateStatusCode(this->statusCode);
	httpHeaders = GenerateHttpResponse(strbodyHtmlLen);
	response.clear();
	response += request.getRequestLine()["Protocol"] + " " + statusCodeLine + "\r\n" + httpHeaders + htmlFile;
	return (response);
}

std::string ServerResponse::GenerateStatusCode(int code)
{
	std::map<int, std::string>::iterator it = this->mapStatusCode.find(code);
	if (it != this->mapStatusCode.end())
		return it->second;
	return "";
}
std::string ServerResponse::responsePostMethod()
{
	std::string response;
	if (request.getTypeBody() == MULTIPART)
		response = handleFilesUploads();
	else if (request.getTypeBody() == TEXT)
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

std::string ServerResponse::responseDeleteMethod()
{
	std::string response = pageNotFound();
	//TODO: check path to resource, if it exits delete, if not send negative response
	std::string pathToResource = info.getServerRootPath() + request.getRequestLine()["Request-URI"];
	std::ifstream file(pathToResource.c_str());
	if (!(file.good()))
		return response;
	remove(pathToResource.c_str());
	response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 0\r\n"
				"Connection: keep-alive\r\n"
				"\r\n"; //very imporant
	return response;
}

std::string ServerResponse::handleFilesUploads()
{
	std::map<std::string, std::string> httpRequestLine;
	std::string response;
	std::map<std::string, std::string> headersBody;
	std::string binaryBody;
	std::vector<struct section> sectionBodies;

	httpRequestLine = request.getRequestLine();
	sectionBodies = request.getSections();
	headersBody = sectionBodies[0].myMap;
	binaryBody = sectionBodies[0].body;
	if (sectionBodies.size() > 1)
	{
		response = "HTTP/1.1 400 Bad Request\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: 0\r\n"
								"Connection: close\r\n"
								"\r\n";
		return response;
	}

	std::string requestTarget = httpRequestLine["Request-URI"];
	requestTarget.erase(requestTarget.begin());
	std::string pathFile = info.getServerRootPath() + requestTarget; //it only works if given this path by the client?
	std::string fileName = getFileName(headersBody);
	std::string fileType = getFileType(headersBody);
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
	pathFile += "/" + fileName;
	int file = open(pathFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0)
	{
		perror("Error opening file");
		return (response);
	}
	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
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

//utilis
std::string ServerResponse::pageNotFound(void)
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

std::string ServerResponse::getFileContent(std::string path)
{
	std::ifstream file;
	std::string line;
	std::string htmlFile;
	std::string temp;

	file.open(path.c_str(), std::fstream::in | std::fstream::out);
	if (!file)
	{
		Logger::error("Failed to open html file: " + std::string(strerror(errno)));
		return (htmlFile);
	}
	while (std::getline(file, line))
	{
		if (line.size() == 0)
			continue;
		else
			htmlFile = htmlFile.append(line + "\r\n");
	}
	file.close();
	return (htmlFile);
}

//TODO: review this function: add couple more headers maybe
std::string ServerResponse::GenerateHttpResponse(std::string length)
{
	std::string httpHeaders;
	std::string contentType;
	std::string fileType;
	std::string fileExtension;

	httpHeaders += "Content-Length: " + length + "\r\n";
	httpHeaders += "Connection: keep-alive\r\n"; //client end connection right away, keep-alive
	httpHeaders += "\r\n";
	return httpHeaders;
}

std::string ServerResponse::getFileType(std::map<std::string, std::string> headers)
{
	std::map<std::string, std::string>::iterator it;
	std::string type;

	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Content-Type")
			type = it->second;
	}
	return type;
}

std::string ServerResponse::getFileName(std::map<std::string, std::string> headers)
{
	std::map<std::string, std::string>::iterator it;
	std::string name;

	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Content-Disposition")
		{
			if (it->second.find("filename") != std::string::npos)
			{
				std::string fileNameField = it->second.substr(it->second.find("filename"));
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
	return name;
}

int ServerResponse::checkNameFile(std::string str, std::string path)
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

void ServerResponse::createMapStatusCode(void)
{
	//2xx is for success
	this->mapStatusCode.insert(std::pair<int, std::string>(200, "200 OK"));
	this->mapStatusCode.insert(std::pair<int, std::string>(201, "201 Created"));
	this->mapStatusCode.insert(std::pair<int, std::string>(202, "202 Accepted"));
	this->mapStatusCode.insert(std::pair<int, std::string>(204, "204 No Content"));

	//3xx is for redirection
	this->mapStatusCode.insert(std::pair<int, std::string>(301, "301 Moved Permanently"));
	this->mapStatusCode.insert(std::pair<int, std::string>(302, "302 Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(304, "304 Not Modified"));

	//4xxis for client error
	this->mapStatusCode.insert(std::pair<int, std::string>(400, "400 Bad Request"));
	this->mapStatusCode.insert(std::pair<int, std::string>(401, "401 Unauthorized"));
	this->mapStatusCode.insert(std::pair<int, std::string>(403, "403 Forbidden"));
	this->mapStatusCode.insert(std::pair<int, std::string>(404, "404 Not Found"));
	this->mapStatusCode.insert(std::pair<int, std::string>(405, "405 Method Not Allowed"));
	this->mapStatusCode.insert(std::pair<int, std::string>(409, "409 Conflict"));

	//5xx for server error
	this->mapStatusCode.insert(std::pair<int, std::string>(500, "500 Internal Server Error"));
	this->mapStatusCode.insert(std::pair<int, std::string>(503, "503 Service Unavailabled"));
}