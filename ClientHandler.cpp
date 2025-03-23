#include "ClientHandler.hpp"

//Constructor and Destructor
// ClientHandler::ClientHandler(int fd, InfoServer info)
// {
// 	this->fd = fd;
// 	this->totbytes = 0;
// 	//this->info = info;
// }

ClientHandler::ClientHandler(int fd, InfoServer const &info)
{
	this->fd = fd;
	this->totbytes = 0;
	this->info = info;
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

int ClientHandler::clientStatus(void)
{
	int result;
	HttpRequest request;

	result = readData(this->fd, this->raw_data, this->totbytes);
	if (result == 0 || result == 1)
		return 1;
	else
	{
		if (this->raw_data.find("GET") != std::string::npos)
		{
			//Logger::debug("raw data: " + raw_data);
			request.HttpParse(this->raw_data, this->totbytes);
			this->request = request;
			Logger::debug("Done parsing");
			//create getRoute(uri)
			//handle if doesn't find path - maybe create exceptions
			struct Route route;
			std::string uri = this->request.getHttpRequestLine()["Request-URI"];
			Logger::debug("uri: " + uri);
			route = this->info.getRoute()[uri];
			std::string localPath = route.path;
			Logger::debug("localpath: " + localPath);
			if (isCgi(uri) == true)
			{
				Logger::info("Set up CGI here");
			}
			else
			{
				std::set<std::string> methods = route.methods;
				std::string method = this->request.getHttpRequestLine()["Method"];
				if (methods.count(method) < 0)
					method = "OTHER";
				this->response = prepareResponse(localPath, uri, method);
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
				Logger::debug("in post section");
				request.HttpParse(this->raw_data, this->totbytes);
				this->request = request;
				Logger::debug("Done parsing");
				//call retrieve route
				struct Route route;
				std::string uri = this->request.getHttpRequestLine()["Request-URI"];
				route = this->info.getRoute()[uri];
				std::string localPath = route.path;
				if (isCgi(this->request.getHttpHeaders()["Request-URI"]) == true)
				{
					Logger::info("Set up CGI here");
				}
				else
				{
					std::set<std::string> methods = route.methods;
					std::string method = this->request.getHttpHeaders()["Method"];
					if (methods.count(method) < 0)
						method = "OTHER";
					this->response = prepareResponse(localPath, uri, method);
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

	Logger::debug("search path: " + path);
	folder = fopen(path.c_str(), "rb");
	if (folder == NULL)
		return false;
	fclose(folder);
	return true;
}

int ClientHandler::isCgi(std::string str)
{
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage("./www/static" + str) == true)
		return false;
	return true;
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

std::string extractContent(std::string path)
{
	std::ifstream inputFile(path.c_str(), std::ios::binary);

		if (!inputFile)
		{
			std::cerr << "Error opening file." << std::endl;
			return "";
		}

		inputFile.seekg(0, std::ios::end);
		std::streamsize size = inputFile.tellg();
		inputFile.seekg(0, std::ios::beg);
		std::string buffer; 
		buffer.resize(size);
		if (inputFile.read(&buffer[0], size))
			std::cout << "Read " << size << " bytes from the file." << std::endl;
		else
			std::cerr << "Error reading file." << std::endl;

		inputFile.close();
		return buffer;
	}

std::string ClientHandler::retrievePage(std::string localPath, std::string uri)
{
	std::string body;
	std::string htmlPage;
	std::string pathToTarget;
	struct stat pathStat;

	localPath.erase(localPath.size() - 1); //need to get this correct
	pathToTarget = localPath + uri;
	Logger::debug(pathToTarget);
	if (stat(pathToTarget.c_str(), &pathStat) != 0)
		Logger::error("Failed stat: " + std::string(strerror(errno)));
	if (S_ISDIR(pathStat.st_mode))
	{
		if (pathToTarget[pathToTarget.length()-1] == '/')
			pathToTarget += "index.html";
		else
		pathToTarget += "/index.html";
	}
	if (fileExists(pathToTarget) == false)
	{
		struct Route route;
		std::string errorHtml = "./www/errors/404.html";
		route = this->info.getRoute()[errorHtml];
		std::string errorPagePath = route.path + errorHtml;
		throw NotFoundException(errorPagePath);
	}
	else
		htmlPage = pathToTarget;
	body = extractContent(htmlPage);
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

std::string ClientHandler::uploadFile(std::string localPath, std::string uri)
{
	std::map<std::string, std::string> httpRequestLine;
	std::string body;
	std::map<std::string, std::string> headersBody;
	std::string binaryBody;
	std::vector<struct section> sectionBodies;

	httpRequestLine = request.getHttpRequestLine();
	sectionBodies = request.getHttpSections();
	headersBody = sectionBodies[0].myMap;
	binaryBody = sectionBodies[0].body;
	if (sectionBodies.size() > 1)
		throw ServiceUnavailabledException("./server_root/public_html/503.html");
	// requestTarget.erase(requestTarget.begin());
	// std::string pathFile = "temp/"+ uri; //what is going on here?
	std::string fileName = getFileName(headersBody);
	std::string fileType = getFileType(headersBody);
	if (checkNameFile(fileName, localPath) == 1)
		throw ConflictException();
	localPath += fileName;
	Logger::debug("path to file " + localPath);
	int file = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0)
	{
		perror("Error opening file");
		throw BadRequestException("./server_root/public_html/400.html");
	}
	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
	if (written < 0) {
		perror("Error writing to file");
		close(file);
		throw BadRequestException("./server_root/public_html/400.html");
	}
	close(file);
	body = extractContent("./server_root/public_html/success/index.html");
	return body;
}

std::string      ClientHandler::deleteFile(std::string localPath, std::string uri)
{
	std::string body;
	std::string pathToResource = "./" + request.getHttpRequestLine()["Request-URI"];
	std::ifstream file(pathToResource.c_str());
	if (!(file.good()))
		throw NotFoundException(pathToResource);
	else
		remove(pathToResource.c_str());
	return body;
}

std::string ClientHandler::prepareResponse(std::string localPath, std::string uri, std::string method)
{
	std::string body;
	std::string response;
	int code;
	try
	{
		if (method == "GET")
		{
			body = retrievePage(localPath, uri);
			Logger::debug("body " + body);
			code = 200;
		}
		else if (method == "POST")
		{
			body = uploadFile(localPath, uri);
			code = 201;
		}
		else if (method == "DELETE")
		{
			body = deleteFile(localPath, uri);
			code = 200;
		}
		else
		{
			// struct Route route = getRoute("/405.html");
			// throw MethodNotAllowedException(route.path);
			throw MethodNotAllowedException("./server_root/public_html/405.html");
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
	HttpResponse http(code, body);
	response = http.composeRespone();
	return response;
}
