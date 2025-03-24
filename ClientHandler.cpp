#include "ClientHandler.hpp"

//Constructor and Destructor
ClientHandler::ClientHandler(int fd, Server &configInfo)
{
	this->fd = fd;
	this->totbytes = 0;
	this->configInfo = configInfo;
}

//Setters and Getters
int ClientHandler::getFd(void) const
{
	return this->fd;
}

HttpRequest ClientHandler::getRequest() const
{
	return this->request;
}

std::string ClientHandler::getResponse() const
{
	return this->response;
}

//Main functions

void printRoute(const Route& route)
{
    std::cout << "Route Information:" << std::endl;
    std::cout << "URI: " << route.uri << std::endl;
    std::cout << "Path: " << route.path << std::endl;

    std::cout << "Methods: ";
    if (route.methods.empty()) {
        std::cout << "None";
    } else {
        for (const auto& method : route.methods) {
            std::cout << method << " ";
        }
    }
    std::cout << std::endl;

    std::cout << "Location Settings:" << std::endl;
    for (const auto& setting : route.locSettings) {
        std::cout << "  " << setting.first << ": " << setting.second << std::endl;
    }

    std::cout << "Internal: " << (route.internal ? "true" : "false") << std::endl;
}

std::string extractUri(std::string str)
{
	int size;
	std::string newStr;
	int i = 0;

	size = str.size();
	for (i = size - 1; i > 0; i--)
	{
		if (str[i] == '/')
			break;
	}
	newStr = str.substr(0, i);
	return newStr;
}

int ClientHandler::clientStatus(void)
{
	int result;
	HttpRequest request;
	std::string uri;
	struct Route route;

	result = readData(this->fd, this->raw_data, this->totbytes);
	if (result == 0 || result == 1)
		return 1;
	else
	{
		if (this->raw_data.find("GET") != std::string::npos || this->raw_data.find("DELETE") != std::string::npos)
		{
			request.HttpParse(this->raw_data, this->totbytes);
			this->request = request;
			Logger::debug("Done parsing GET/DELETE");
			//do useful checks for http request being correct: http, no transfer encoding
			uri = this->request.getHttpRequestLine()["Request-URI"];
			if (uri.find("index.html") != std::string::npos) //little tricky, to review
				uri.erase(uri.size()-11);
			if (this->raw_data.find("DELETE") != std::string::npos)
			{
				Logger::debug("extract file to delete and create new uri");
				std::string newUri;
				newUri = extractUri(uri);
				Logger::debug(newUri);
				uri.clear();
				uri = newUri;
			}
			if (isCgi(uri) == true)
			{
				Logger::info("Set up CGI");
			}
			else
			{
					//create function to mathc the uri to route, handle if not found - not i am checking it
					//path should be without ending / since it comes with the uri
					route = configInfo.getRoute()[uri];
					if (route.path.empty())
					{
						struct Route errorPage;

						errorPage = this->configInfo.getRoute()["/404.html"];
						std::string errorBody = extractContent(errorPage.path);
						HttpResponse http(404, errorBody);
						this->response = http.composeRespone();
						this->totbytes = 0;
						this->raw_data.clear();
						return 2;
					}
					this->response = prepareResponse(this->request, route);
					this->totbytes = 0;
					this->raw_data.clear();
					Logger::info("Response created successfully and store in clientQueu");
					return 2;
			}
		}
		else if (this->raw_data.find("Content-Length") != std::string::npos)
		{
			int start = this->raw_data.find("Content-Length") + 16;
			int end = this->raw_data.find("\r\n", this->raw_data.find("Content-Length"));
			int bytes_expected = Utils::toInt(this->raw_data.substr(start, end - start));
			if (this->totbytes >= bytes_expected)
			{
				request.HttpParse(this->raw_data, this->totbytes);
				this->request = request;
				Logger::debug("Done parsing");

				uri = this->request.getHttpRequestLine()["Request-URI"];
				route = configInfo.getRoute()[uri];
				int maxBodySize;
				maxBodySize = Utils::toInt(route.locSettings.find("limit_client_body_size")->second);
				if (bytes_expected > maxBodySize * 1000000)
				{
					struct Route errorPage;
					errorPage = this->configInfo.getRoute()["/413.html"];
					std::string errorBody = extractContent(errorPage.path);
					HttpResponse http(413, errorBody);
					this->response = http.composeRespone();
					this->totbytes = 0;
					this->raw_data.clear();
					return 2;
				}
				if (isCgi(uri) == true)
				{
					Logger::info("Set up CGI");
				}
				else
				{
					this->response = prepareResponse(this->request, route);
					this->totbytes = 0;
					this->raw_data.clear();
					Logger::info("Response created successfully and store in clientQueu");
					return 2;
				}
			}
		}
	}
	return 0;
}

int ClientHandler::readData(int fd, std::string &str, int &bytes)
{
	int res = 0;
	char buffer[BUFFER];

	while (1)
	{
		Utils::ft_memset(buffer, 0, sizeof(buffer));
		res = recv(fd, buffer, BUFFER, MSG_DONTWAIT);
		if (res <= 0)
			break;
		str.append(buffer, res);
		bytes += res;
	}
	if (res == 0)
		return 0;
	else if (res == -1 && bytes == 0)
		return 1;
	return 2;
}

int ClientHandler::retrieveResponse(void)
{
	int bytes = send(fd, this->response.c_str(), this->response.size(), 0);
	if (bytes == -1)
		return -1;
	this->response.clear();
	this->raw_data.clear();
	this->totbytes = 0;
	this->request.cleanProperties();
	return 0;
}

int	searchPage(std::string path)
{
	FILE *folder;

	folder = fopen(path.c_str(), "rb");
	if (folder == NULL)
		return false;
	fclose(folder);
	return true;
}

int ClientHandler::isCgi(std::string str)
{
	return false;
}

bool fileExists(std::string path)
{
	std::ifstream file;
	std::string line;
	std::string htmlFile;
	std::string temp;

	file.open(path.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
	if (!file)
	{
		Logger::error("Failed to open html file: " + std::string(strerror(errno)));
		return (false);
	}
	file.close();
	return true;
}

std::string ClientHandler::extractContent(std::string path)
{
	std::ifstream inputFile(path.c_str(), std::ios::binary);

		if (!inputFile)
		{
			std::cout << path << std::endl;
			std::cerr << "Error opening file." << std::endl;
			return "";
		}

		inputFile.seekg(0, std::ios::end);
		std::streamsize size = inputFile.tellg();
		inputFile.seekg(0, std::ios::beg);
		std::string buffer; 
		buffer.resize(size);
		if (!(inputFile.read(&buffer[0], size)))
			std::cerr << "Error reading file." << std::endl;

		inputFile.close();
		return buffer;
	}

std::string ClientHandler::retrievePage(std::string uri, struct Route route)
{
	std::string body;
	struct stat pathStat;

	//function to retrieve route from uri
	(void)uri; //really i dont need to concatenate
	if (stat(route.path.c_str(), &pathStat) != 0)
		Logger::error("Failed stat: " + std::string(strerror(errno)));
	if (S_ISDIR(pathStat.st_mode))
	{
		if (route.path[route.path.length()-1] == '/')
			route.path += route.locSettings.find("index")->second;
		else
			route.path += "/index.html"; //add index in other location too
	}
	body = extractContent(route.path);
	return body;
}

std::string getFileType(std::map<std::string, std::string> headers)
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

std::string getFileName(std::map<std::string, std::string> headers)
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

int checkNameFile(std::string str, std::string path)
{
	DIR *folder;
	struct dirent *data;

	folder = opendir(path.c_str());
	std::string convStr;
	std::cout << path << std::endl;
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

std::string ClientHandler::uploadFile(HttpRequest request, std::string path)
{
	std::string body;
	std::map<std::string, std::string> headersBody;
	std::string binaryBody;
	std::vector<struct section> sectionBodies;
	struct Route page;

	sectionBodies = request.getHttpSections();
	headersBody = sectionBodies[0].myMap;
	binaryBody = sectionBodies[0].body;
	if (sectionBodies.size() > 1)
	{
		page = this->configInfo.getRoute()["/503.html"];
		throw ServiceUnavailabledException(page.path);
	}
	std::string fileName = getFileName(headersBody);
	std::string fileType = getFileType(headersBody);
	if (checkNameFile(fileName, path) == 1)
		throw ConflictException();
	path += "/" + fileName;
	int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0)
	{
		page = this->configInfo.getRoute()["/400.html"];
		throw BadRequestException(page.path);
	}
	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
	if (written < 0) {
		perror("Error writing to file");
		close(file);
		page = this->configInfo.getRoute()["/400.html"];
		throw BadRequestException(page.path);
	}
	close(file);
	page = this->configInfo.getRoute()["/success"];
	body = extractContent(page.path + "/index.html"); //to review how to extract wanted page
	return body;
}

std::string      ClientHandler::deleteFile(std::string path)
{
	std::string body;
	std::ifstream file(path.c_str());

	if (!(file.good()))
	{
		struct Route errorPage;
		errorPage = this->configInfo.getRoute()["/404.html"];
		throw NotFoundException(errorPage.path);
	}
	else
		remove(path.c_str());
	return body;
}

std::string extraFileName(std::string str)
{
	std::string newStr;
	int size;

	size = str.size();
	int i = 0;
	for (i = size - 1; i > 0; i--)
	{
		if (str[i] == '/')
			break;
	}
	newStr = str.substr(i);
	return newStr;
}

std::string ClientHandler::prepareResponse(HttpRequest request, struct Route route)
{
	std::string body;
	std::string response;
	int code;
	std::string method;
	struct Route errorPage;
	
	method = this->request.getHttpRequestLine()["Method"];
	try
	{
		std::string uri = request.getHttpRequestLine()["Request-URI"];
		if (method == "GET" && route.methods.count(method) > 0)
		{
			body = retrievePage(uri, route);
			code = 200;
		}
		else if (method == "POST" && route.methods.count(method) > 0)
		{
			body = uploadFile(request, route.path);
			code = 201;
		}
		else if (method == "DELETE" && route.methods.count(method) > 0)
		{
			std::string fileToDelete;
			std::string fileName;
			fileName = extraFileName(this->request.getHttpRequestLine()["Request-URI"]);
			fileToDelete = route.path + fileName;
			body = deleteFile(fileToDelete);
			code = 204;
		}
		else
		{
			errorPage = this->configInfo.getRoute()["/405.html"];
			throw MethodNotAllowedException(errorPage.path);
		}
	}
	catch(const NotFoundException& e)
	{
		Logger::error(e.what());
		code = e.getCode();
		body = e.getBody();
	}
	catch(const MethodNotAllowedException& e)
	{
		Logger::error(e.what());
		code = e.getCode();
		body = e.getBody();
	}
	catch(const BadRequestException& e)
	{
		Logger::error(e.what());
		code = e.getCode();
		body = e.getBody();
	}
	catch(const ServiceUnavailabledException& e)
	{
		Logger::error(e.what());
		code = e.getCode();
		body = e.getBody();
	}
	catch(const ConflictException& e)
	{
		Logger::error(e.what());
		code = e.getCode();
		body = e.getBody();
	}
	HttpResponse http(code, body);
	response = http.composeRespone();
	return response;
}
