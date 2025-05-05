#include "ClientHandler.hpp"
#include "PrintFunction.hpp"
#include "CGI.hpp"

#define INCOMPLETE 0
//Constructor
ClientHandler::ClientHandler(int fd, InfoServer const &configInfo)
{
	this->client_fd = fd;
	this->totbytes = 0;
	this->configInfo = configInfo;
	this->startingTime = time(NULL);
	this->timeoutTime = atof(this->configInfo.getSetting()["keepalive_timeout"].c_str());
	this->internal_fd = 0;
	this->pid = 0;
	std::string errorPath = this->configInfo.getSetting()["error_path"];
	HttpException::setHtmlRootPath(errorPath);
}

//Setters and Getters

void ClientHandler::setResponse(std::string str)
{
	this->response = str;
}

int ClientHandler::getFd(void) const
{
	return this->client_fd;
}

int ClientHandler::getPid(void) const
{
	return this->pid;
}

int ClientHandler::getCGI_Fd(void) const
{
	return this->internal_fd;
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

double ClientHandler::getTimeOut(void) const
{
	return this->timeoutTime;
}

//Main functions

void ClientHandler::validateHttpHeaders(struct Route route)
{
	std::string uri = this->request.getHttpRequestLine()["request-uri"];
	std::string method = this->request.getHttpRequestLine()["method"];
	int	method_count = route.methods.size();
	//Logger::debug("For URI: " + uri + " Method: " + method + " and count " + Utils::toString(method_count) + " ");
	//Check if method is allowed
	if ((method_count == 0) | (route.methods.find(method) == route.methods.end()))
		throw MethodNotAllowedException();
	// route = this->configInfo.getRoute()[uri];
	std::map<std::string, std::string> headers = this->request.getHttpHeaders();
	std::map<std::string, std::string>::iterator it;
	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-type")
		{
			std::string type;
			if (it->second.find("multipart/form-data") != std::string::npos)
			{
				std::string genericType = it->second.substr(0, it->second.find(";"));
				type = this->request.getHttpSection().myMap["content-type"];
				if (type.length() >= 1)
					type.erase(type.length() - 1);
			}
			else
				type = it->second;
			std::map<std::string, std::string>::iterator itC;
			for (itC = route.locSettings.begin(); itC != route.locSettings.end(); itC++)
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
		else if (it->first == "upgrade")
			throw HttpVersionNotSupported();
	}
}

std::string ClientHandler::findDirectory(std::string uri)
{
	struct Route route;
	size_t index;
	route = this->configInfo.getRoute()[uri];
	if (route.path.empty())
	{
		index = uri.find(".");
		if (index != std::string::npos)
		{
			index = uri.find_last_of("/");
			uri = uri.substr(0, index);
			route = this->configInfo.getRoute()[uri];
			if (!route.path.empty())
				return uri;
		}
		while (uri.size() > 1)
		{
			index = uri.find_last_of("/");
			if (index != std::string::npos)
				uri = uri.substr(0, index);
			else
				break;
			route = this->configInfo.getRoute()[uri];
			if (!(route.path.empty()))
				return uri;
		}
	}
	else
		return (uri);
	return "/";
}

std::string ClientHandler::createPath(struct Route route, std::string uri)
{
	std::string lastItem;
	std::string path = route.path;
	std::string defaultFile = route.locSettings["index"];
	size_t index;

	if (path == "/")
		return "";
	if (path[path.size() - 1] == '/')
		path.erase(path.size() - 1, 1);
	index = uri.find_last_of("/");
	if (index != std::string::npos)
		lastItem = uri.substr(index + 1);
	if (defaultFile.empty())
		path += "/" + lastItem;
	else
		path += uri;
	return path;
}

void ClientHandler::validateHttpHeaders(struct Route route)
{
	std::string uri = this->request.getHttpRequestLine()["request-uri"];
	route = this->configInfo.getRoute()[uri];
	std::map<std::string, std::string> headers = this->request.getHttpHeaders();
	std::map<std::string, std::string>::iterator it;
	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-type")
		{
			std::string type;
			if (it->second.find("multipart/form-data") != std::string::npos)
			{
				std::string genericType = it->second.substr(0, it->second.find(";"));
				type = this->request.getHttpSection().myMap["content-type"];
				if (type.length() >= 1)
					type.erase(type.length() - 1);
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
		else if (it->first == "upgrade")
			throw HttpVersionNotSupported();
	}
}

int ClientHandler::checkRequestStatus(void)
{
	std::string stringLowerCases = this->raw_data;
	std::transform(stringLowerCases.begin(), stringLowerCases.end(), stringLowerCases.begin(), Utils::toLowerChar);
	if (stringLowerCases.find("\r\n\r\n") == std::string::npos)
		return 0;
	if (stringLowerCases.find("content-length") != std::string::npos && stringLowerCases.find("post") != std::string::npos)
	{
		int start = stringLowerCases.find("content-length") + 16;
		int end = stringLowerCases.find("\r\n", this->raw_data.find("content-length"));
		int bytes_expected = Utils::toInt(this->raw_data.substr(start, end - start));
		// if (bytes_expected > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"]))
		// 	throw PayLoadTooLargeException();
		std::string onlyHeaders = this->raw_data.substr(0, this->raw_data.find("\r\n\r\n"));
		if (this->totbytes < bytes_expected + onlyHeaders.size())
			return 0;
	}
	if (stringLowerCases.find("transfer-encoding") != std::string::npos)
	{
		Logger::info("transfer encoding");
		size_t emptyLines = stringLowerCases.find("\r\n\r\n");
		if (stringLowerCases.find("0\r\n", emptyLines) == std::string::npos)
			return 0;
		else
		{
			std::string body = stringLowerCases.substr(stringLowerCases.find("\r\n\r\n") + 4);
			int bytes_expected = body.size();
			if (bytes_expected > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"]))
				throw PayLoadTooLargeException();
		}
	}
	return 1;
}


void ClientHandler::updateRoute(struct Route &route)
{
	if (route.locSettings.find("alias") != route.locSettings.end())
	{
		route.path.clear();
		route.path += "." + route.locSettings.find("alias")->second;
		int count = std::count(this->request.getUri().begin(), this->request.getUri().end(), '/');
		if (count >= 2)
		{
			std::string copyUri = this->request.getUri();
			copyUri.erase(0, 1);
			size_t index = copyUri.find_first_of("/");
			std::string file = copyUri.substr(index + 1);
			route.path += "/" + file;
		}
		struct stat pathStat;
		if (stat(route.path.c_str(), &pathStat) == 0)
		{
			if (S_ISDIR(pathStat.st_mode))
			{
				if (route.locSettings.find("default") != route.locSettings.end())
				{
					route.path += "/" + route.locSettings.find("default")->second;
				}
			}
		}
	}
}

void ClientHandler::redirectClient(struct Route &route)
{
	struct Route redirectRoute;
	redirectRoute = this->configInfo.getRoute()[route.locSettings.find("redirect")->second];
	if (redirectRoute.path.empty())
		findPath(route.locSettings.find("redirect")->second, redirectRoute);
	route.path.clear();
	route.path = redirectRoute.path;
	route.methods = this->configInfo.getRoute()[redirectRoute.locSettings.find("redirect")->second].methods;
	HttpResponse http(Utils::toInt(route.locSettings.find("status")->second), "");
	http.setUriLocation(redirectRoute.uri);
	this->response = http.composeRespone();
}

void ClientHandler::findPath(std::string str, struct Route &route)
{
	std::string locationPath;
	locationPath = findDirectory(str);
	struct Route newRoute;
	newRoute = this->configInfo.getRoute()[locationPath];
	route = newRoute;
	std::string newPath;
	newPath = createPath(route, str);
	route.path.clear();
	route.path = newPath;
}

int ClientHandler::manageRequest()
{
	int result;
	std::string uri;
	struct Route route;
	result = readData(this->client_fd, this->raw_data, this->totbytes);
	if (result == 0 || result == 1)
		return 1;
	else
	{
		try
		{
			if (checkRequestStatus() == INCOMPLETE)
				return 0;
			Logger::info("Done receving request");
			this->request.HttpParse(this->raw_data, this->totbytes);
			Logger::info("Done parsing");
			uri = this->request.getHttpRequestLine()["request-uri"];
			route = configInfo.getRoute()[uri];
			if (route.uri.empty())
				findPath(uri, route);
			updateRoute(route);
			Logger::info("Got route");
			validateHttpHeaders(route);
			Logger::info("Validate http request");
			if (isCgi(route.uri) == true)
			{
				Logger::info("Set up CGI");
				std::string	upload_dir;
				if (route.locSettings.find("upload_dir") != route.locSettings.end())
					upload_dir = "." + route.locSettings["upload_dir"];
				else
					upload_dir = "";
				if (access(route.path.c_str(), F_OK) != 0)
					throw NotFoundException();
				CGI	cgi(request, upload_dir, route.path, configInfo);
				internal_fd = cgi.getFD();
				this->raw_data.clear();
				pid = cgi.getPid();
				return 3;
			}
			else
			{
				if (route.locSettings.find("redirect") != route.locSettings.end())
					redirectClient(route);
				else
					this->response = prepareResponse(route);
				Logger::info("It is static");
				Logger::info("Response created successfully and store in clientQueu");
			}
		}
		catch (const HttpException &e)
		{
			HttpResponse http(e.getCode(), e.getBody());
			this->response = http.composeRespone();
			Logger::error(Utils::toString(e.getCode()) + " " + e.what());
		}
		this->totbytes = 0;
		this->raw_data.clear();
		return 2;
	}
	return 0;
}

int ClientHandler::readData(int fd, std::string &str, int &bytes)
{
	int res = 0;
	char buffer[BUFFER];

	Utils::ft_memset(buffer, 0, sizeof(buffer));
	Logger::error("buffer error memset");
	res = recv(fd, buffer, BUFFER, MSG_DONTWAIT);
	if (res > 0)
		str.append(buffer, res);
	bytes += res;
	if (res == 0)
		return 0;
	else if (res == -1 && bytes == 0)
		return 1;
	return 2;
}

int ClientHandler::retrieveResponse(void)
{
	int bytes = send(client_fd, this->response.c_str(), this->response.size(), 0);
	if (bytes == -1)
		return -1;
	this->response.clear();
	this->raw_data.clear();
	this->totbytes = 0;
	this->request.cleanProperties();
	this->startingTime = time(NULL);
	return this->internal_fd;
}

int ClientHandler::isCgi(std::string uri)
{
	if (uri == "/cgi-bin")
		return true;
	return false;
}

std::string ClientHandler::extractContent(std::string path)
{
	std::ifstream inputFile(path.c_str(), std::ios::binary);
	if (!inputFile)
		throw NotFoundException();
	inputFile.seekg(0, std::ios::end);
	std::streamsize size = inputFile.tellg();
	inputFile.seekg(0, std::ios::beg);
	std::string buffer;
	buffer.resize(size);
	if (!(inputFile.read(&buffer[0], size)))
		throw NotFoundException();
	inputFile.close();
	return buffer;
}

std::string ClientHandler::retrievePage(struct Route route)
{
	std::string body;
	if (route.path.find("index") == std::string::npos)
	{
		if (route.locSettings.find("index") != route.locSettings.end())
		{
			if (route.path[route.path.size() - 1] == '/')
				route.path += route.locSettings.find("index")->second;
			else
				route.path += "/" + route.locSettings.find("index")->second;
		}
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
		throw NotFoundException();
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
	std::string fileName;

	sectionBody = request.getHttpSection();
	headersBody = sectionBody.myMap;
	binaryBody = sectionBody.body;
	if (binaryBody.empty())
	{
		fileName = "file";
		std::string type = this->request.getHttpHeaders()["content-type"];
		size_t index = type.find("/");
		type = type.substr(index + 1);
		fileName += "." + type;
		binaryBody = this->request.getBodyContent();
	}
	else
	{
		fileName = getFileName(headersBody);
		std::string fileType = getFileType(headersBody);
	}
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
	page = this->configInfo.getRoute()["/"];
	body = extractContent(page.path + "success_upload" + "/" + page.locSettings.find("index")->second);
	return body;
}

std::string      ClientHandler::deleteFile(std::string path)
{
	std::string body;
	std::ifstream file(path.c_str());

	if (!(file.good()))
		throw NotFoundException();
	else
		std::remove(path.c_str());
	return body;
}

std::string ClientHandler::prepareResponse(struct Route route)
{
	std::string body;
	std::string response;
	int code = 200;
	std::string method;
	struct Route errorPage;

	if (route.internal == true)
		throw NotFoundException();
	method = this->request.getHttpRequestLine()["method"];
	if (method == "GET" && route.methods.count(method) > 0)
	{
		body = retrievePage(route);
		code = 200;
	}
	else if (method == "POST" && route.methods.count(method) > 0)
	{
		body = uploadFile(route.path);
		code = 201;
	}
	else if (method == "DELETE" && route.methods.count(method) > 0)
	{
		body = deleteFile(route.path);
		code = 204;
	}
	// else
	// 	throw MethodNotAllowedException();
	HttpResponse http(code, body);
	response = http.composeRespone();
	return response;
}

int ClientHandler::readStdout(int fd)
{
	int res = 0;
	char buffer[BUFFER];
	Utils::ft_memset(buffer, 0, sizeof(buffer));
	res = read(fd, buffer, BUFFER - 1);
	this->raw_data.append(buffer, res);
	if (res > 0)
		return 0;
	else if (res == -1 && this->totbytes == 0)
		return 1;
	return 2;
}

int ClientHandler::createResponse(void)
{
	try
	{
		int code = 200;
		if (this->raw_data.find("<html") != std::string::npos || this->raw_data.find("<!DOCTYPE") != std::string::npos)
		{
			HttpResponse http(code, this->raw_data);
			this->response = http.composeRespone();
		}
		else
		{
			if (this->raw_data.find("Content-Type") == std::string::npos)
				throw ServiceUnavailabledException();
			if (this->raw_data.find("\n\n") == std::string::npos)
				throw ServiceUnavailabledException();
			size_t index_status = this->raw_data.find("Status");
			std::string statusLine;
			size_t index_end_line;
			size_t index_start_number;
			size_t index_end_number;
			std::string codeStr;
			if (index_status == std::string::npos)
			{
				if (this->request.getMethod() == "POST")
					code = 201;
			}
			else
			{
				index_end_line = this->raw_data.find("\n");
				if (index_end_line == std::string::npos)
					throw ServiceUnavailabledException();
				statusLine = this->raw_data.substr(index_status, index_end_line);
				index_start_number = statusLine.find(" ");
				if (index_start_number == std::string::npos)
					throw ServiceUnavailabledException();
				statusLine = statusLine.substr(index_start_number + 1);
				index_end_number = statusLine.find(" ");
				if (index_end_number == std::string::npos)
					throw ServiceUnavailabledException();
				codeStr = statusLine.substr(0, index_end_line - 1);
				code = Utils::toInt(codeStr);
			}
			size_t index = this->raw_data.find("\n\n");
			std::string headers;
			headers = this->raw_data.substr(0, index + 2);
			std::string body;
			body = this->raw_data.substr(index + 2);
			HttpResponse http(code, body);
			this->response = http.composeRespone(headers);
		}
	}
	catch (const HttpException &e)
	{
		HttpResponse http(e.getCode(), e.getBody());
		this->response = http.composeRespone();
		Logger::error(Utils::toString(e.getCode()) + " " + e.what());
	}
	return 2;
}
