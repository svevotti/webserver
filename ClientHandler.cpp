#include "ClientHandler.hpp"

//Constructor and Destructor
ClientHandler::ClientHandler(int fd, InfoServer &configInfo)
{
	this->fd = fd;
	this->totbytes = 0;
	this->configInfo = configInfo;
	this->startingTime = time(NULL);
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

double ClientHandler::getTime() const
{
	return this->startingTime;
}
//Main functions

// void printRoute(const Route& route)
// {
//     std::cout << "Route Information:" << std::endl;
//     std::cout << "URI: " << route.uri << std::endl;
//     std::cout << "Path: " << route.path << std::endl;

//     std::cout << "Methods: ";
//     if (route.methods.empty()) {
//         std::cout << "None";
//     } else {
//         for (const auto& method : route.methods) {
//             std::cout << method << " ";
//         }
//     }
//     std::cout << std::endl;

//     std::cout << "Location Settings:" << std::endl;
//     for (const auto& setting : route.locSettings) {
//         std::cout << "  " << setting.first << ": " << setting.second << std::endl;
//     }

//     std::cout << "Internal: " << (route.internal ? "true" : "false") << std::endl;
// }

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

void ClientHandler::validateHttpHeaders(void)
{
	struct Route route;

	std::string uri = this->request.getHttpRequestLine()["request-uri"];
	route = this->configInfo.getRoute()[uri];
	std::map<std::string, std::string> headers = this->request.getHttpHeaders();
	std::map<std::string, std::string>::iterator it;
	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-type")
		{
			std::string type;
			if (it->second.find(";") != std::string::npos)
			{
				std::string genericType = it->second.substr(0, it->second.find(";"));
				if (genericType == "multipart/form-data")
				{
					type = this->request.getHttpSection().myMap["content-type"];
					if (type.length() >= 1)
						type.erase(type.length() - 1);
				}
				else
					throw UnsupportedMediaTypeException();
			}
			else
				type = it->second;
			std::map<std::string, std::string>::iterator itC;
			for (itC = route.locSettings.begin(); itC != route.locSettings.end(); it++)
			{
				if (itC->first == "content_type")
				{
					if (itC->second.find(type) != std::string::npos)
						return;
					else
						throw UnsupportedMediaTypeException();
				}
			}
		}
		else if (it->first == "transfer-encoding")
			throw NotImplementedException();
		else if (it->first == "Upgrade")
			throw HttpVersionNotSupported();
	}
}

int ClientHandler::clientStatus(void)
{
	int result;
	// HttpRequest request;
	std::string uri;
	struct Route route;
	result = readData(this->fd, this->raw_data, this->totbytes);
	if (result == 0 || result == 1)
		return 1;
	else
	{
		std::string stringLowerCases = this->raw_data;
		std::transform(stringLowerCases.begin(), stringLowerCases.end(), stringLowerCases.begin(), Utils::toLowerChar);
		if (stringLowerCases.find("get") != std::string::npos || stringLowerCases.find("delete") != std::string::npos)
		{
			try
			{
				this->request.HttpParse(this->raw_data, this->totbytes);
			}
			catch(const BadRequestException& e)
			{
				Logger::error(e.what());
				HttpResponse http(e.getCode(), e.getBody());
				this->response = http.composeRespone();
				this->totbytes = 0;
				this->raw_data.clear();
				return 2;
			}
			Logger::info("Done http parsing");
			try
			{
				validateHttpHeaders();
			}
			catch(UnsupportedMediaTypeException& e)
			{
				Logger::error(e.what());
				HttpResponse http(e.getCode(), e.getBody());
				this->response = http.composeRespone();
				this->totbytes = 0;
				this->raw_data.clear();
				return 2;
			}
			catch(NotImplementedException& e)
			{
				Logger::error(e.what());
				HttpResponse http(e.getCode(), e.getBody());
				this->response = http.composeRespone();
				this->totbytes = 0;
				this->raw_data.clear();
				return 2;
			}
			catch(HttpVersionNotSupported& e)
			{
				Logger::error(e.what());
				HttpResponse http(e.getCode(), e.getBody());
				this->response = http.composeRespone();
				this->totbytes = 0;
				this->raw_data.clear();
				return 2;
			}
			//do useful checks for http request being correct: http, no transfer encoding
			uri = this->request.getHttpRequestLine()["request-uri"];
			//create function to find subdirectories
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
					//create logic to retrieve prefix uri
					// if (this->request.getHttpRequestLine().count("query-string") > 0)
					// {
					// 	struct Route errorPage;

					// 	errorPage = this->configInfo.getRoute()["/404.html"];
					// 	std::string errorBody = extractContent(errorPage.path);
					// 	HttpResponse http(404, errorBody);
					// 	this->response = http.composeRespone();
					// 	this->totbytes = 0;
					// 	this->raw_data.clear();
					// 	return 2;
					// }
					route = configInfo.getRoute()[uri];
					// printRoute(route);
					if (route.locSettings.find("redirect") != route.locSettings.end())
					{
						//Logger::warn("should be here");
						route.path.clear();
						route.path = this->configInfo.getRoute()[route.locSettings.find("redirect")->second].path;
						route.methods = this->configInfo.getRoute()[route.locSettings.find("redirect")->second].methods;
						//Logger::debug(route.path);
						//printRoute(route);
						HttpResponse http(301, "");
						this->response = http.composeRespone();

					}
					else if (route.path.empty())
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
					else
						this->response = prepareResponse(route);
					this->totbytes = 0;
					this->raw_data.clear();
					Logger::info("Response created successfully and store in clientQueu");
					return 2;
			}
		}
		else if (stringLowerCases.find("content-length") != std::string::npos)
		{
			int start = stringLowerCases.find("content-length") + 16;
			int end = stringLowerCases.find("\r\n", this->raw_data.find("content-length"));
			int bytes_expected = Utils::toInt(this->raw_data.substr(start, end - start));
			if (bytes_expected > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"]) * 1000000)
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
			if (this->totbytes >= bytes_expected)
			{
				this->request.HttpParse(this->raw_data, this->totbytes);
				Logger::debug("Done parsing");
				try
				{
					validateHttpHeaders();
				}
				catch(UnsupportedMediaTypeException& e)
				{
					Logger::error(e.what());

					HttpResponse http(e.getCode(), e.getBody());
					this->response = http.composeRespone();
					this->totbytes = 0;
					this->raw_data.clear();
					return 2;
				}
				catch(NotImplementedException& e)
				{
					Logger::error(e.what());

					HttpResponse http(e.getCode(), e.getBody());
					this->response = http.composeRespone();
					this->totbytes = 0;
					this->raw_data.clear();
					return 2;
				}
				catch(HttpVersionNotSupported& e)
				{
					Logger::error(e.what());
					HttpResponse http(e.getCode(), e.getBody());
					this->response = http.composeRespone();
					this->totbytes = 0;
					this->raw_data.clear();
					return 2;
				}
				uri = this->request.getHttpRequestLine()["request-uri"];
				route = configInfo.getRoute()[uri];
				if (isCgi(uri) == true)
				{
					Logger::info("Set up CGI");
				}
				else
				{
					this->response = prepareResponse(route);
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
		if (it->first == "content-type")
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
		if (it->first == "content-disposition")
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

std::string ClientHandler::uploadFile(std::string path)
{
	std::string body;
	std::map<std::string, std::string> headersBody;
	std::string binaryBody;
	struct section sectionBody;
	struct Route page;

	sectionBody = request.getHttpSection();
	headersBody = sectionBody.myMap;
	binaryBody = sectionBody.body;

	std::string fileName = getFileName(headersBody);
	std::string fileType = getFileType(headersBody);
	if (checkNameFile(fileName, path) == 1)
		throw ConflictException();
	path += "/" + fileName;
	int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0)
		throw BadRequestException();
	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
	if (written < 0) {
		perror("Error writing to file");
		close(file);
		throw BadRequestException();
	}
	close(file);
	page = this->configInfo.getRoute()["/success"];
	body = extractContent(page.path + "/index.html"); //to review how to extract wanted page
	return body;
}

// std::string ClientHandler::uploadFile(HttpRequest request, std::string path)
// {
// 	std::string body;
// 	std::map<std::string, std::string> headersBody;
// 	std::string binaryBody;
// 	std::vector<struct section> sectionBodies;
// 	struct Route page;

// 	sectionBodies = request.getHttpSections();
// 	headersBody = sectionBodies[0].myMap;
// 	binaryBody = sectionBodies[0].body;
// 	if (sectionBodies.size() > 1)
// 	{
// 		std::cout << "here" << std::endl;
// 		throw ServiceUnavailabledException();
// 	}
// 	std::string fileName = getFileName(headersBody);
// 	std::string fileType = getFileType(headersBody);
// 	if (checkNameFile(fileName, path) == 1)
// 		throw ConflictException();
// 	path += "/" + fileName;
// 	int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
// 	if (file < 0)
// 		throw BadRequestException();
// 	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
// 	if (written < 0) {
// 		perror("Error writing to file");
// 		close(file);
// 		throw BadRequestException();
// 	}
// 	close(file);
// 	page = this->configInfo.getRoute()["/success"];
// 	body = extractContent(page.path + "/index.html"); //to review how to extract wanted page
// 	return body;
// }

std::string      ClientHandler::deleteFile(std::string path)
{
	std::string body;
	std::ifstream file(path.c_str());

	if (!(file.good()))
		throw NotFoundException();
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

std::string ClientHandler::prepareResponse(struct Route route)
{
	std::string body;
	std::string response;
	int code;
	std::string method;
	struct Route errorPage;
	
	method = this->request.getHttpRequestLine()["method"];
	try
	{
		std::string uri = request.getHttpRequestLine()["request-uri"];
		if (method == "GET" && route.methods.count(method) > 0)
		{
			body = retrievePage(uri, route);
			if (route.locSettings.find("redirect") != route.locSettings.end())
				code = 301;
			else
				code = 200;
		}
		else if (method == "POST" && route.methods.count(method) > 0)
		{
			body = uploadFile(route.path);
			code = 201;
		}
		else if (method == "DELETE" && route.methods.count(method) > 0)
		{
			std::string fileToDelete;
			std::string fileName;
			fileName = extraFileName(this->request.getHttpRequestLine()["request-uri"]);
			fileToDelete = route.path + fileName;
			body = deleteFile(fileToDelete);
			code = 204;
		}
		else
			throw MethodNotAllowedException();
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
