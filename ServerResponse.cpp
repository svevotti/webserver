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

typedef struct header
{
	std::map<std::string, std::string> myMap;
	std::vector<char> binaryData;
} header;

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
	if (!request["Request-target"].empty())
	{

		std::string documentRootPath = info.getServerDocumentRoot();
		std::string pathToTarget = documentRootPath.substr(0, documentRootPath.length() - 1) + request["Request-target"];

		struct stat pathStat;

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
		headers = assembleHeaders(request["Protocol"], page, strbodyHtmlLen);
		response.clear();
		response += headers + bodyHtml;
	}
	else
		std::cout << "Missing request target" << std::endl;
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
	// printf("count %d\n", count);
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

std::string ServerResponse::responsePostMethod(InfoServer info, std::map<std::string, std::string> request, const char *buffer, int size)
{
	
	//parse body
	std::cout << "\033[36mExtract data\033[0m" << std::endl;
	std::string contentType = request["Content-Type"];
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		std::vector<int> boundariesIndexes;
		//find boundary		
		char *b;
		std::cout << "\033[36mGet boundary\033[0m" << std::endl;
		b = getBoundary(buffer);
		int blen = strlen(b);
		std::cout << "\033[36mGet boundary index in vector\033[0m" << std::endl;
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
		std::cout << "\033[36mGet sections in body\033[0m" << std::endl;
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
					// printf("here\n");
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

		//build path where to store uploads/images/user_uploads
		std::string pathToUploads = info.getServerRootPath() + "uploads/"; //it is a folder
		//check which kind of file it is: look for filename in content-disposition header of each section.
		std::map<int, struct header>::iterator outerIt;
		std::map<std::string, std::string>::iterator innerIt;
		// int i = 0;
		for (outerIt = bodySections.begin(); outerIt != bodySections.end(); outerIt++)
		{
			// printf("i %d\n", i++);
			//std::cout << "Index: " << outerIt->first << std::endl;
			struct header section = outerIt->second;

			std::string fileName;
			for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
			{
				// std::cout << "index in vector: " << binaryDataIndex[0] << std::endl;
				if (innerIt->first == "Content-Disposition")
				{
					if (innerIt->second.find("filename") != std::string::npos)
					{
						std::string fileNameField = innerIt->second.substr(innerIt->second.find("filename"));
						if (fileNameField.find('"') != std::string::npos)
						{
							int indexFirstQuote = fileNameField.find('"');
							int indexSecondQuote;
							if (fileNameField.find('"', indexFirstQuote+1) != std::string::npos)
							{
								indexSecondQuote = fileNameField.find('"', indexFirstQuote+1);
							}
							//std::cout << indexFirstQuote << std::endl;
							//std::cout << indexSecondQuote << std::endl;
							fileName = fileNameField.substr(indexFirstQuote+1,indexSecondQuote - indexFirstQuote-1);
							//std::cout << fileName << std::endl;
						}
					}
				}
				std::cout << innerIt->first << std::endl;
				std::cout << innerIt->second << std::endl;
			}
		}
		
		//openfile
		std::cout << "\033[36mOpen file\033[0m" << std::endl;
		std::string pathToFile = info.getServerRootPath() + "images/data.jpg";
		std::cout << pathToFile << std::endl;
		int file = open(pathToFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (file < 0) {
			perror("Error opening file");
			exit(1);
		}
		//write to the file
		// std::cout << binaryDataIndex[0] << std::endl;
		std::cout << "\033[36mWrite to file\033[0m" << std::endl;
		std::cout << size << std::endl;
		ssize_t written = write(file, buffer + binaryDataIndex[0] + 2, size - binaryDataIndex[0] - 2);
		std::cout << "\033[36mAfter writing\033[0m" << std::endl;
		if (written < 0) {
			perror("Error writing to file");
			close(file);
			exit(1);
		}
		std::cout << "\033[36mBefore closing file\033[0m" << std::endl;
		close(file);
		std::cout << "\033[36mAfter closing file\033[0m" << std::endl;
	}
	else
	{
		//store in one string
		std::cout << "message is " << request["Body"] << std::endl;
	}	//we can fake i saved and store the image when it is already there
	std::cout << "\033[36mBuilding response\033[0m" << std::endl;
	std::string response =
					"HTTP/1.1 200 OK\r\n"
					"Content-Length: 0\r\n"
					"Connection: keep-alive\r\n"
					"\r\n"; //very imporant
	std::cout << "\033[36mBefore returning statment parsing and responding\033[0m" << std::endl;
	return (response);
}
