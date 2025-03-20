#include "ClientHandler.hpp"

//Constructor and Destructor
ClientHandler::ClientHandler(int fd, InfoServer info)
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
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage(this->info.getServerDocumentRoot() + str) == true)
		return false;
	return true;
}

HttpRequest ParsingRequest(std::string str, int size)
{
	HttpRequest request;
	request.HttpParse(str, size);
	return request;
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

std::string ClientHandler::retrievePage(HttpRequest request)
{
	std::string body;
	std::string htmlPage;
	std::ostringstream intermediatestream;
	std::string strbodyHtmlLen;
	std::string httpHeaders;
	std::string statusCodeLine;
	std::string documentRootPath;
	std::string pathToTarget;
	struct stat pathStat;
	std::map<std::string, std::string> httpRequestLine;

	httpRequestLine = request.getHttpRequestLine();
	documentRootPath = this->info.getServerDocumentRoot();
	pathToTarget = documentRootPath + httpRequestLine["Request-URI"];
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
		throw NotFoundException("./server_root/public_html/404.html");
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

std::string ClientHandler::uploadFile(HttpRequest request)
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
	std::string requestTarget = httpRequestLine["Request-URI"];
	requestTarget.erase(requestTarget.begin());
	std::string pathFile = this->info.getServerRootPath() + "/" + requestTarget;
	std::string fileName = getFileName(headersBody);
	std::string fileType = getFileType(headersBody);
	if (checkNameFile(fileName, pathFile) == 1)
		throw ConflictException();
	pathFile += "/" + fileName;
	int file = open(pathFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
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

std::string      ClientHandler::deleteFile(HttpRequest request)
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

std::string ClientHandler::prepareResponse(HttpRequest request)
{
	std::string body;
	std::string response;
	int code;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request.getHttpRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		try
		{
			if (httpRequestLine["Method"] == "GET")
			{
				body = retrievePage(request);
				Logger::debug("body " + body);
				code = 200;
			}
			else if (httpRequestLine["Method"] == "POST")
			{
				body = uploadFile(request);
				code = 201;
			}
			else if (httpRequestLine["Method"] == "DELETE")
			{
				body = deleteFile(request);
				code = 200;
			}
			else
				throw MethodNotAllowedException("./server_root/public_html/405.html");
		}
		catch(const NotFoundException& e)
		{
			Logger::error(e.what());
			code = e.getCode();
			body = e.getBody();
		}
		HttpResponse http(code, body);
		response = http.composeRespone();
	}
	else
		Logger::error("Method not found, Sveva, use correct status code line");
	return response;
}

int ClientHandler::clientStatus(void)
{
	int result;

	result = readData(this->fd, this->raw_data, this->totbytes);
	if (result == 0 || result == 1)
		return 1;
	else
	{
		if (this->raw_data.find("GET") != std::string::npos)
		{
			this->request = ParsingRequest(this->raw_data, this->totbytes);
			if (isCgi(this->request.getHttpHeaders()["Request-URI"]) == true)
				return 3;
			else
			{
				this->response = prepareResponse(this->request);
				this->totbytes = 0;
				this->raw_data.clear();
				Logger::info("Response created successfully and store in clientQueu");
				return 2;
			}
		}
		else if (this->raw_data.find("Content-Length") != std::string::npos)
		{
			if (this->totbytes >= 2873) //don't forget to extract the actual size from conentent length
			{
				this->request = ParsingRequest(this->raw_data, this->totbytes);
				Logger::debug("Done parsing");
				if (isCgi(this->request.getHttpHeaders()["Request-URI"]) == true)
					return 3;
				else
				{
					this->response = prepareResponse(this->request);
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
