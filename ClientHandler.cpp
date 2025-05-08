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
void ClientHandler::setGateawayResponse()
{
	std::string body = extractContent("." + this->configInfo.getSetting()["error_path"] + "/504.html");
	HttpResponse http(504, body);
	this->response = http.composeRespone("");
}

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
		if (bytes_expected > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"])) //Potential source of errors
			throw PayLoadTooLargeException();
		std::string onlyHeaders = this->raw_data.substr(0, this->raw_data.find("\r\n\r\n"));
		if (this->totbytes < (int) (bytes_expected + onlyHeaders.size())) //Potential source of errors
			return 0;
	}
	if (stringLowerCases.find("transfer-encoding") != std::string::npos)
	{
		//Logger::info("transfer encoding");
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
	this->response = http.composeRespone("");
}

std::string ClientHandler::createBodyError(int code, std::string str)
{
	struct Route errorRoute;
	std::string body;

	errorRoute = this->configInfo.getRoute()["/" + Utils::toString(code)];
	if (!errorRoute.uri.empty())
	{
		std::string path;
		std::string file;
		path = errorRoute.path.substr(0, errorRoute.path.find_last_of("/") + 1);
		if (errorRoute.locSettings.find("return") != errorRoute.locSettings.end())
		{
			file = errorRoute.locSettings.find("return")->second;
		}
		else
			return str;
		path += file;
		//Logger::debug(path);
		body = extractContent(path);
	}
	else
		body = str;
	return body;
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
			//Logger::info("Done receving request");
			this->request.HttpParse(this->raw_data, this->totbytes);
			//Logger::info("Done parsing");
			uri = this->request.getHttpRequestLine()["request-uri"];
			route = configInfo.getRoute()[uri];
			if (route.uri.empty())
				findPath(uri, route);
			updateRoute(route);
			//Logger::info("Got route");
			validateHttpHeaders(route);
			//Logger::info("Validate http request");
			if (isCgi(route.uri) == true)
			{
				//Logger::info("Set up CGI");
				std::string	upload_dir;
				if (route.locSettings.find("upload_dir") != route.locSettings.end())
					upload_dir = "." + route.locSettings["upload_dir"];
				else
					upload_dir = "";
				if (access(route.path.c_str(), F_OK) != 0)
					throw NotFoundException();
				if (access(route.path.c_str(), X_OK) != 0)
					throw ForbiddenException();
				CGI	cgi(request, upload_dir, route.path, configInfo);
				internal_fd = cgi.getFD();
				this->raw_data.clear();
				pid = cgi.getPid();
				return 3;
			}
			else
			{
				if (route.locSettings.find("redirect") != route.locSettings.end())
				{
					struct Route redirectRoute;
					redirectRoute = this->configInfo.getRoute()[route.locSettings.find("redirect")->second];
					if (redirectRoute.path.empty())
					{
						std::string locationPath;
						locationPath = findDirectory(route.locSettings.find("redirect")->second);
						struct Route newRoute;
						newRoute = this->configInfo.getRoute()[locationPath];
						redirectRoute = newRoute;
						std::string newPath;
						newPath = createPath(redirectRoute, route.locSettings.find("redirect")->second);
						redirectRoute.path.clear();
						redirectRoute.path = newPath;
					}
					route.path.clear();
					route.path = redirectRoute.path;
					route.methods = this->configInfo.getRoute()[redirectRoute.locSettings.find("redirect")->second].methods;
					HttpResponse http(Utils::toInt(route.locSettings.find("status")->second), "");
					http.setUriLocation(redirectRoute.uri);
					this->response = http.composeRespone("");
				}
				else
					this->response = prepareResponse(route);
				//Logger::info("It is static");
				//Logger::info("Response created successfully and store in clientQueu");
			}
		}
		catch (const HttpException &e)
		{
			std::string body;

			body = createBodyError(e.getCode(), e.getBody());
			HttpResponse http(e.getCode(), body);
			this->response = http.composeRespone("");
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
	return bytes;
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
	if ((int) binaryBody.size() > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"])) //Potential source of errors
			throw PayLoadTooLargeException();
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
	if (method == "GET")
	{
		body = retrievePage(route);
		code = 200;
	}
	else if (method == "POST")
	{
		body = uploadFile(route.path);
		code = 201;
	}
	else if (method == "DELETE")
	{
		body = deleteFile(route.path);
		code = 204;
	}
	// else
	// 	throw MethodNotAllowedException();
	HttpResponse http(code, body);
	response = http.composeRespone("");
	return response;
}

int ClientHandler::readStdout(int fd)
{
	int res = 0;
	char buffer[BUFFER];
	///if don't read all from the pipe right away it goes to POLLHUP
	while (1)
	{
		Utils::ft_memset(buffer, 0, sizeof(buffer));
		res = read(fd, buffer, BUFFER - 1);
		if (res == 0)
			break;
		this->raw_data.append(buffer, res);
	}
	//Logger::debug("bytes read: " + Utils::toString(this->raw_data.size()));
	// if (res > 0)
	// 	return 0;
	if (res == -1 && this->totbytes == 0)
		return 1;
	return 2;
}

int ClientHandler::createResponse(void)
{
	try
	{
		int code = 0;
		std::string body;
		std::string headers;

		size_t index_new_line = this->raw_data.find("\n\n");
		code = extractStatusCode(this->raw_data, this->request.getMethod());
		if (index_new_line != std::string::npos)
		{
			headers = this->raw_data.substr(0, index_new_line + 2);
			body = this->raw_data.substr(index_new_line + 2);
		}
		else
			body = this->raw_data;
		HttpResponse http(code, body);
		this->response = http.composeRespone(headers);
	}
	catch (const HttpException &e)
	{
		HttpResponse http(e.getCode(), e.getBody());
		this->response = http.composeRespone("");
		Logger::error(Utils::toString(e.getCode()) + " " + e.what());
	}
	return 2;
}
