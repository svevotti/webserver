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

typedef struct header
{
	std::map<std::string, std::string> myMap;
	std::vector<char> binaryData;
} header;

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
	statusLine += "Content-Type: ";
	// if (fileName.find(".") != std::string::npos)
	// {
	// 	fileExtension = fileName.substr(fileName.find(".") + 1);
	// 	contentType = getContentType(fileExtension);
	// }
	statusLine += contentType + "\r\n";
	statusLine += "Content-Length: " + length + "\r\n";
	statusLine += "Connection: keep-alive\r\n"; //client end connection right away, keep-alive
	statusLine += "\r\n";
	return statusLine;
}

std::string ServerResponse::responseGetMethod(InfoServer info, std::map<std::string, std::string> request)
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

	if (!request["Request-target"].empty())
	{
		documentRootPath = info.getServerDocumentRoot();
		pathToTarget = documentRootPath.substr(0, documentRootPath.length() - 1) + request["Request-target"];
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
		headers = assembleHeaders(request["Protocol"], strbodyHtmlLen);
		response.clear();
		response += headers + bodyHtml;
	}
	else
		response = pageNotFound();
	return (response);
}

char *getBoundary(const char *buffer)
{
	char *b;
	const char *result = strstr(buffer, "=");
	result++;
	int index_start = result - buffer;
	int count = 0;
	while (1)
	{
		if (*result == '\r')
			break ;
		count++;
		result++;
	}
	int i = 0;
	b = new char[count+1];
	while (i < count)
	{
		b[i] = buffer[index_start];
		i++;
		index_start++;
	}
	b[i] = '\0';
	return (b);
}

std::string idetifyFile(std::string str)
{
	std::string extension;

	if (str.find(".") != std::string::npos)
	{
		extension = str.substr(str.find(".")+1);
	}
	return extension;
}

std::string createUniqueName(std::string str)
{
	time_t timeNow = time(0);
	struct tm *timeInfo;
	std::string name;
	std::string extension;
	if (str.find(".") != std::string::npos)
	{
		name = str.substr(0, str.find("."));
		extension = str.substr(str.find("."));
	}
	std::string fileName = "user_" + name + "_";
	timeInfo = localtime(&timeNow);
	std::vector<int> timestamp;
	timestamp.push_back(timeInfo->tm_year + 1900);
	timestamp.push_back(timeInfo->tm_mon + 1);
	timestamp.push_back(timeInfo->tm_mday);
	timestamp.push_back(timeInfo->tm_hour);        
	timestamp.push_back(timeInfo->tm_min);
	timestamp.push_back(timeInfo->tm_sec);

	std::ostringstream strStream;
	for (int i = 0; i < 3; i++)
	{
		strStream << timestamp[i];
	}
	fileName += strStream.str() + "_";
	strStream.str(""); //clear content
	strStream.clear(); //clear
	for (int i = 3; i < (int)timestamp.size(); i++)
	{
		strStream << timestamp[i];
	}
	fileName += strStream.str() + extension;
	return (fileName);
}

std::string getFolder(std::string str)
{
	std::string	fileExtText[] = {"html", "css", "txt"};
	std::string fileExtApp[] = {"json", "javascript", "pdf"};
	std::string fileExtImg[] = {"png", "gif", "jpg"};
	std::string fileType;
	std::string ext;
	if (str.find(".") != std::string::npos)
	{
		ext = str.substr(str.find(".")+1);
	}
	int i = 0;
	
	for(i = 0; i < 3; i++)
	{
		if (fileExtText[i] == ext)
			fileType = "/docs/";
		else if (fileExtApp[i] == ext)
			fileType = "/applications/";
		else if (fileExtImg[i] == ext)
			fileType = "/images/";
	}
	return (fileType);
}

std::string ServerResponse::responsePostMethod(InfoServer info, std::map<std::string, std::string> request, const char *buffer, int size)
{
	
	std::string contentType = request["Content-Type"];
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		std::vector<int> boundariesIndexes;
		char *b;
		b = getBoundary(buffer);
		int blen = strlen(b);
		for (int i = 0; i < size; i++) 
		{
			int diff = ft_memcmp(buffer + i, b, blen);
			// printf("diff: %d")
			if (diff == 0) {
				boundariesIndexes.push_back(i);
				// printf("boundary index: %d for %s\n", i, buffer + i);
			}
		}
		std::string line;
		struct header data;
		std::map<int, struct header> bodySections;
		std::string key;
		std::string value;
		std::vector<int> binaryDataIndex;
		int indexBinary = 0;
		for (int i = 1; i < (int)boundariesIndexes.size() - 1; i++) //excluding first and last
		{
			std::istringstream streamHeaders(buffer + boundariesIndexes[i]);
			indexBinary = boundariesIndexes[i];
			while (getline(streamHeaders, line))
			{
				if (line.find_first_not_of("\r\n") == std::string::npos)
					break ;
				indexBinary += line.length() + 1;
				if (line.find(b) != std::string::npos)
					continue;
				else
				{
					if (line.find(":") != std::string::npos)
						key = line.substr(0, line.find(":"));
					if (line.find(" ") != std::string::npos)
						value = line.substr(line.find(" ") + 1);
					data.myMap[key] = value;
					bodySections[i] = data;
				}
			}
			binaryDataIndex.push_back(indexBinary);
			streamHeaders.clear();
			line.clear();
			key.clear();
			value.clear();
			indexBinary = 0;
		}

		//build path where to store-> root or /uploads?
		std::string requestTarget = request["Request-target"];
		requestTarget.erase(requestTarget.begin());
		std::string pathFile = info.getServerRootPath() + requestTarget; //it only works if given this path by the client?

		//check which kind of file it is: just if there is one section
		std::map<int, struct header>::iterator outerIt;
		std::map<std::string, std::string>::iterator innerIt;
		// int i = 0;
		std::string fileName;
		//printf("looking for filename0\n");
		std::string fileType;
		if (bodySections.size() == 1)
		{
			for (outerIt = bodySections.begin(); outerIt != bodySections.end(); outerIt++)
			{
				struct header section = outerIt->second;
				for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
				{
					//look for name of file
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
								fileName = fileNameField.substr(indexFirstQuote+1,indexSecondQuote - indexFirstQuote-1);
							}
						}
					}
					else if (innerIt->first == "Content-Type")
					{
						fileType = innerIt->second;
					}
				}
			}
		}
		else
		{
			std::string response = "HTTP/1.1 400 Bad Request\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: 0\r\n"
									"Connection: close\r\n"
									"\r\n";
		}
		//openfile
		std::cout << "filename: " << fileName << std::endl;
		std::cout << "filetype: " << fileType << std::endl;
		//check if file exists already
		DIR *folder;
		struct dirent *info;
		folder = opendir(pathFile.c_str());
		std::string convStr;
		if (folder == NULL)
			printf("error opening folder\n");
		while ((info = readdir(folder)))
		{
			convStr = info->d_name;
			if (convStr == fileName)
			{
				std::cout << "File with name already existing, please change it\n";
				std::string response = "HTTP/1.1 400 Bad Request\r\n"
										"Content-Type: text/plain\r\n"
										"Content-Length: 0\r\n"
										"Connection: close\r\n"
										"\r\n";
				break;
			}
		}
		closedir(folder);
		pathFile += "/" + fileName;
		std::cout << pathFile << std::endl;
		int file = open(pathFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (file < 0) {
			perror("Error opening file");
			exit(1);
		}
		//write to the file
		ssize_t written = write(file, buffer + binaryDataIndex[0] + 2, size - binaryDataIndex[0] - 2);
		if (written < 0) {
			perror("Error writing to file");
			close(file);
			exit(1);
		}
		close(file);
	}
	else
	{
		//store in one string
		std::cout << "message is " << request["Body"] << std::endl;
	}	//we can fake i saved and store the image when it is already there
	std::string response =
					"HTTP/1.1 200 OK\r\n"
					"Content-Length: 0\r\n"
					"Connection: keep-alive\r\n"
					"\r\n"; //very imporant
	return (response);
}

std::string ServerResponse::responseDeleteMethod(InfoServer info, std::map<std::string, std::string> request)
{
	std::cout << "Delete method" << std::endl;

	std::string response = pageNotFound();
	std::map<std::string, std::string>::iterator it;
	for (it = request.begin(); it != request.end(); it++)
	{
		std::cout << it->first << " : ";
		std::cout << it->second << std::endl;
	}
	//check what is the resource to be delete (in target request)
	std::cout << "target: " << request["Request-target"] << std::endl;
	//TODO: check path to resource, if it exits delete, if not send negative response
	std::string pathToResource = info.getServerRootPath() + request["Request-target"];
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