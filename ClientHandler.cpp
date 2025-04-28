#include "ClientHandler.hpp"
#include "PrintFunction.hpp"

//Constructor and Destructor
ClientHandler::ClientHandler(int fd, InfoServer const &configInfo)
{
	this->fd = fd;
	this->totbytes = 0;
	this->configInfo = configInfo;
	//NEW: +33 lines
	//Simona has changed it to ensure microsecond precision (gettimeofday), checkf that keepalive_timeout exists and if not it sets it to a default
	//this->startingTime = time(NULL);
	//this->timeoutTime = atof(this->configInfo.getSetting()["keepalive_timeout"].c_str());
	struct timeval tv;
	gettimeofday(&tv, NULL);
	this->startingTime = tv.tv_sec + tv.tv_usec / 1000000.0;

	std::map<std::string, std::string> settings = configInfo.getSetting();
	//For debugging - remove?
	Logger::debug("Settings size: " + Utils::toString(settings.size()) + " for FD " + Utils::toString(fd));
	std::map<std::string, std::string>::const_iterator it = settings.find("keepalive_timeout");
	if (it == settings.end())
	{
		Logger::warn("No keepalive_timeout in settings for FD " + Utils::toString(fd) + ", using default 75");
		this->timeoutTime = 75.0;
	}
	else
	{
		this->timeoutTime = atof(it->second.c_str());
		//For debugging - remove?
		Logger::debug("Set timeoutTime to " + Utils::toString(this->timeoutTime) + " for FD " + Utils::toString(fd));
	}
	//This part is neccesary to integrate the CGI part
	this->cgiProcessingTimeout = configInfo.getCGIProcessingTimeout();
	//For debugging - remove?
	Logger::debug("Set cgiProcessingTimeout to " + Utils::toString(this->cgiProcessingTimeout) +
			" for FD " + Utils::toString(fd) + " on server " + configInfo.getIP() + ":" + configInfo.getPort());

	//std::string errorPath = this->configInfo.getSetting()["error_path"];
	it = settings.find("error_path");
	std::string errorPath = it != settings.end() ? it->second : "";
	//For debugging - remove?
	Logger::debug("Error path: " + errorPath + " for FD " + Utils::toString(fd));
	HttpException::setHtmlRootPath(errorPath);
	//NEW: +6 lines
	//For debugging - remove?
	Logger::debug("ClientHandler constructed for FD " + Utils::toString(fd));
	//For debugging - remove?
	Logger::debug("Creating ClientHandler for server " + configInfo.getIP() + ":" + configInfo.getPort() +
	" with settings: keepalive_timeout=" + Utils::toString(this->timeoutTime) +
	", cgi_processing_timeout=" + Utils::toString(this->cgiProcessingTimeout));
}

//NEW: Full function
// Copy Constructor
ClientHandler::ClientHandler(const ClientHandler& other)
	: fd(other.fd), totbytes(other.totbytes), raw_data(other.raw_data),
	startingTime(other.startingTime), timeoutTime(other.timeoutTime), configInfo(other.configInfo),
	request(other.request), response(other.response),
	cgiProcessingTimeout(other.cgiProcessingTimeout)
{
	Logger::debug("Copying ClientHandler for FD " + Utils::toString(fd));
}

//NEW: Full function
// Copy Assignment Operator // to be rechecked
ClientHandler& ClientHandler::operator=(const ClientHandler& other)
{
	if (this != &other)
	{
		fd = other.fd;
		totbytes = other.totbytes;
		raw_data = other.raw_data;
		startingTime = other.startingTime;
		timeoutTime = other.timeoutTime;
		cgiProcessingTimeout = other.cgiProcessingTimeout;
		configInfo = other.configInfo;
		request = other.request;
		response = other.response;
		Logger::debug("Assigning ClientHandler for FD " + Utils::toString(fd));
	}
	return *this;
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

double ClientHandler::getTimeOut(void) const
{
	return this->timeoutTime;
}

//NEW: Function
double ClientHandler::getCGIProcessingTimeout(void) const
{
	return configInfo.getCGIProcessingTimeout();
}

//NEW: Function
void ClientHandler::setResponse(const std::string& resp)
{
	this->response = resp;
}
//Main functions

void ClientHandler::validateHttpHeaders(struct Route route)
{
	std::string uri = this->request.getHttpRequestLine()["request-uri"];

	//NEW: +17 lines
	//TEST: This part is needed for CGI, checks that the method is valid and throws an exception if not.
	std::string method = this->request.getHttpRequestLine()["method"];
	//For debugging - remove?
	Logger::debug("Validating HTTP method: " + this->request.getHttpRequestLine()["method"] + " for URI: " + uri); // Simona debug

	//Making sure method is a valid HTTP method (not a boundary marker)
	if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT")
	{
		Logger::error("Invalid HTTP method: " + method);
		throw BadRequestException();
	}
	// Simona - CGI integration -  Check if method is allowed for this route
	if (route.methods.count(method) == 0)
	{
		Logger::error("Method " + method + " not allowed for URI " + uri);
		throw MethodNotAllowedException();
	}

	route = this->configInfo.getRoute()[uri];
	std::map<std::string, std::string> headers = this->request.getHttpHeaders();
	std::map<std::string, std::string>::iterator it;
	for (it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-type")
		{
			std::string type;
			//NEW: Added an if loop (second.find(";")), if deleting, restore line immediately above and remove lines marked with "if deleting"
			if (it->second.find(";") != std::string::npos)
			//if (it->second.find("multipart/form-data") != std::string::npos)
			{
				std::string genericType = it->second.substr(0, it->second.find(";"));
				if (genericType == "multipart/form-data") //if deleting
				{ //if deleting
					type = this->request.getHttpSection().myMap["content-type"];
					if (type.length() >= 1)
						type.erase(type.length() - 1);
				} //if deleting
				else //if deleting
					throw UnsupportedMediaTypeException(); //if deleting
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
		//else if (it->first == "transfer-encoding") //SVEVA: Removed?
		//	throw NotImplementedException(); //SVEVA: Removed?
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
	//index = lastItem.find("."); //SVEVA: Removed?
	//if (index != std::string::npos && defaultFile.empty()) //SVEVA: Removed?
	if (defaultFile.empty())
		path += "/" + lastItem;
	else
		path += uri;
	return path;
}

int ClientHandler::manageRequest(void)
{
	int result;
	std::string uri;
	struct Route route;
	//For debugging - remove?
	Logger::debug("Processing request for FD " + Utils::toString(fd) + ", raw_data size: " + Utils::toString(raw_data.size())); // Simona's
	result = readData(this->fd, this->raw_data, this->totbytes);
	//For debugging - remove?
	Logger::debug("readData result: " + Utils::toString(result) + ", totbytes: " + Utils::toString(totbytes)); // Simona's log
	if (result == 0 || result == 1)
		return 1;
	else
	{
		std::string stringLowerCases = this->raw_data;
		std::transform(stringLowerCases.begin(), stringLowerCases.end(), stringLowerCases.begin(), Utils::toLowerChar);
		try
		{
			if (stringLowerCases.find("\r\n\r\n") == std::string::npos)
				return 0;
			if (stringLowerCases.find("content-length") != std::string::npos && stringLowerCases.find("post") != std::string::npos)
			{
				int start = stringLowerCases.find("content-length") + 16;
				int end = stringLowerCases.find("\r\n", this->raw_data.find("content-length"));
				int bytes_expected = Utils::toInt(this->raw_data.substr(start, end - start));
				//NEW: + 13 lines
				//if (bytes_expected > Utils::toInt(this->configInfo.getSetting()["client_max_body_size"]) * 1000000) //If restoring, the * 1000000 needs to be removed as it was changed to bytes now
				//	throw PayLoadTooLargeException();
				//Add a default 10MB just for safety (optional)
				size_t maxSize = 10 * 1024 * 1024;
				// From Simona: I broke it down in 2 steps just because i was getting trouble tracking but same code
				std::map<std::string, std::string> settings = this->configInfo.getSetting();
				if (settings.find("client_max_body_size") != settings.end())
					maxSize = Utils::toInt(settings["client_max_body_size"]);
				if (static_cast<size_t>(bytes_expected) > maxSize)
				{
					Logger::error("413 - Payload Too Large: Request body size " + Utils::toString(bytes_expected) + " exceeds client_max_body_size " + Utils::toString(maxSize));
					throw PayLoadTooLargeException();
				}
				if (this->totbytes < bytes_expected)
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
			Logger::info("Done receiving request");
			//For debugging - remove?
			Logger::debug("Parsing request for FD " + Utils::toString(fd));
			this->request.HttpParse(this->raw_data, this->totbytes);
			Logger::info("Done parsing");

			uri = this->request.getHttpRequestLine()["request-uri"];
			//NEW: +2 lines
			//For CGI
			std::string method = this->request.getMethod();
			route = configInfo.getRoute()[uri];
			if (route.uri.empty())
			{
				std::string locationPath;
				locationPath = findDirectory(uri);
				struct Route newRoute;
				newRoute = this->configInfo.getRoute()[locationPath];
				route = newRoute;
				std::string newPath;
				newPath = createPath(route, uri);
				route.path.clear();
				route.path = newPath;
			}
			if (route.locSettings.find("alias") != route.locSettings.end())
			{
				route.path.clear();
				route.path += "." + route.locSettings.find("alias")->second;
				int count = std::count(uri.begin(), uri.end(), '/');
				if (count >= 2)
				{
					std::string copyUri = uri;
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
			Logger::info("Got route");

			//NEW: +6 lines
			// Check for 405 - Method Validation - Simona - New block - check here for CGI integration (does not compromise static)
			if (route.methods.count(method) == 0)
			{
				Logger::error("405 - Method Not Allowed: " + method + " for URI " + uri);
				throw MethodNotAllowedException();
			}

			validateHttpHeaders(route);
			Logger::info("Validate http request");
			if (isCgi(uri) == true)
			{
				//NEW: +4 lines
				//Logger::info("Set up CGI");
				//return 0;
				Logger::info("Set up CGI for URI: " + uri + " on Server " + configInfo.getIP() + ":" + configInfo.getPort());
				return 3;
			}
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
				this->response = http.composeResponse();
			}
			else
				this->response = prepareResponse(route);
			Logger::info("It is static");
			Logger::info("Response created successfully and store in clientQueue");
		}
		catch (const HttpException &e)
		{
			//NEW: +1 line
			Logger::error("Exception in manageRequest for FD " + Utils::toString(fd) + ": " + Utils::toString(e.getCode()) + " " + e.what());
			HttpResponse http(e.getCode(), e.getBody());
			this->response = http.composeResponse();
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

	while (1)
	{
		//NEW: +2 lines
		//Utils::ft_memset(buffer, 0, sizeof(buffer));
		std::memset(buffer, 0, sizeof(buffer));
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

//NEW: Function, before it was just a return false; unsure if Simona is done with it, but I believe so
int ClientHandler::isCgi(std::string str)
{
	std::map<std::string, Route> routes = configInfo.getRoute();
	Route cgiRoute = configInfo.getCGI();
	// New today
	std::string routeKey = findDirectory(str);
	std::map<std::string, Route>::const_iterator routeIt = routes.find(routeKey);

	// Check route-specific CGI settings
	if (routeIt != routes.end() && routeIt->second.locSettings.find("cgi") != routeIt->second.locSettings.end())
	{
		std::string cgiExt = routeIt->second.locSettings.find("cgi")->second;
		if (str.find(cgiExt) != std::string::npos)
		{
			Logger::debug("CGI detected via route settings for URI: " + str);
			return true;
		}
	}

	// Check global CGI route
	if (!cgiRoute.uri.empty() && str.find(cgiRoute.uri) == 0)
	{
		Logger::debug("CGI detected via global CGI route for URI: " + str);
		return true;
	}

	// Check for script extensions
	if (str.find(".py") != std::string::npos ||
		str.find(".php") != std::string::npos)
	{
		Logger::debug("CGI detected via file extension for URI: " + str);
		return true;
	}
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
		throw NotFoundException(); //TEST: Should this be NotFound or Service Unavailable? Should add an input.close inside this if?
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
		throw NotFoundException(); //TEST: Should it be not found or Service Unavailable?
	while ((data = readdir(folder)))
	{
		convStr = data->d_name;
		if (convStr == str)
		{
			//NEW: +1 line
			closedir(folder);
			return (1);
		}
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
	std::string fileType;

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
		fileType = getFileType(headersBody);
	}
	if (checkNameFile(fileName, path) == 1)
		throw ConflictException();
	path += "/" + fileName;
	//For debugging - remove?
	Logger::debug("Uploading file: " + fileName + " of type: " + fileType + " to path: " + path); // Simona debug
	int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (file < 0)
		throw BadRequestException();
	ssize_t written = write(file, binaryBody.c_str(), binaryBody.length());
	if (written < 0) {
		perror("Error writing to file"); //TEST: Should we use the logger?
		close(file);
		throw BadRequestException();
	}
	close(file);
	page = this->configInfo.getRoute()["/"];
	body = extractContent(page.path + "success_upload" + "/" + page.locSettings.find("index")->second); //need to take care
	return body;
}

std::string ClientHandler::deleteFile(std::string path)
{
	std::string body;
	std::ifstream file(path.c_str());

	if (!(file.good()))
		throw NotFoundException();
	else
		remove(path.c_str());
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
	else
		throw MethodNotAllowedException();
	HttpResponse http(code, body);
	//NEW: +6 lines - Is this old code? It was in Simona's, but I think we don't use anymore
	// if (route.uri.find("/images") != std::string::npos)
	// {
	// 	std::string file;
	// 	file = route.path.substr(route.path.find_last_of(".") + 1);
	// 	http.setImageType(file);
	// }
	response = http.composeResponse();
	return response;
}
