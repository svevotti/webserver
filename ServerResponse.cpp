#include "ServerResponse.hpp"
#include <fstream>
#include <dirent.h>
#include <sstream>
#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>

int	ft_memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned char	*p1;
	unsigned char	*p2;
	// int				subs;

	p1 = (unsigned char *)s1;
	p2 = (unsigned char *)s2;
	while (n > 0)
	{
		if (*p1 != *p2)
			return (-1);
		p1++;
		p2++;
		n--;
	}
	return (0);
}

std::string	getHtmlPage(std::string path, std::string target)
{
	struct dirent *folder;
	std::string page;
	std::string str;
	DIR *dir;

	std::cout << "path - \n" << path << std::endl;
	std::cout << "target - \n" << target << std::endl;
	dir = opendir(path.c_str());
	if (dir == NULL)
		std::cerr << "Error in opening directory" << std::endl;
	while ((folder = readdir(dir)) != NULL)
	{
		std::cout << "content: " << folder->d_name << std::endl;
		std::cout << "target: " << target << std::endl;
		std::string str(folder->d_name);
		std::cout << "str: " << str << std::endl;
		if (str == target)
		{
			std::cout << "here" << std::endl;
			page = str;
			break;
		}
	}
	closedir(dir);
	std::cout << "page before returning - " << page << std::endl;
	return (page);
}

std::string getFileContent(std::string fileName)
{
	std::fstream file;
	std::string line;
	std::string bodyHtml;
	
	// std::cout << "path to index " << fileName << std::endl;
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

	(void)info;
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
		bodyHtml = getFileContent(request["Request-target"]);
		// std::cout << "bodyhtml: " << bodyHtml << std::endl;
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
	else
		std::cout << "emptiness" << std::endl;
	//it means page is empty and i should throw some error code
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
	printf("count %d\n", count);
	int i = 0;
	b = (char *)malloc(sizeof(char) * (count + 1));
	while (i < count - 1)
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
	std::cout << "\033[36mStart to parse body\033[0m" << std::endl;
	std::string contentType = request["Content-Type"];
	// printf("buffer %s\n", buffer);
	if (contentType.find("boundary") != std::string::npos) //multi format data
	{
		//find boundary		
		char *b;
		b = getBoundary(buffer);
		int blen = strlen(b);
		printf("boundary: %s, %d\n", b, blen);
		//check if all boundaries are found
		for (int i = 0; i < size; i++) {
			int diff = ft_memcmp(buffer + i, b, blen);
			// printf("diff: %d")
			if (diff == 0) {
				printf("boundary index: %d for %s\n", i, buffer + i);
			}
		}
		//find index of empty line between header and body
		char new_line[] = "\r\n\r\n";
		int last_new_line = 0;
		for (int i = 0; i < size; i++) {
			int diff = ft_memcmp(buffer + i, new_line, 4);
			// printf("diff: %d")
			if (diff == 0) {
				printf("boundary index: %d\n", i);
				last_new_line = i;
				break ;
			}
		}
		// printf("last_new_line: %d\n", last_new_line);
		//open file
		std::string pathToFile = info.getServerRootPath() + "images/data.jpg";
		std::cout << pathToFile << std::endl;
		int file = open(pathToFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (file < 0) {
			perror("Error opening file");
			exit(1);
		}

		//find empty line in the section between header and body
		int header_size = last_new_line + 2;
		for (int i = header_size + 1; i < size; i++) {
			int diff = ft_memcmp(buffer + i, new_line, 4);
			// printf("diff: %d")
			if (diff == 0) {
				// printf("boundary index: %d\n", i);
				last_new_line = i;
				break ;
			}
		}
		// printf("header_size: %d\n", header_size);
		printf("last_new_line in section %d\n", last_new_line);
		//write to the file
		ssize_t written = write(file, buffer + 190, size - 190 - 28);
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
					"Connection: close\r\n"
					"\r\n"; //very imporant
	return (response);
}
