#include "ClientHandler.hpp"

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

int ClientHandler::isCgi(std::string uri)
{
	if (uri == "/cgi-bin")
		return true;
	return false;
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
	int status;
	DIR *folder;
	struct dirent *data;

	status = 0;
	folder = opendir(path.c_str());
	std::string convStr;
	if (folder == NULL)
		throw NotFoundException();
	while ((data = readdir(folder)))
	{
		convStr = data->d_name;
		if (convStr == str) {
			status = 1;
			break ;
		}
	}
	closedir(folder);
	return (status);
}

int extractStatusCode(std::string str, std::string method)
{
	size_t index_status = str.find("Status");
	std::string statusLine;
	size_t index_end_line;
	size_t index_start_number;
	size_t index_end_number;
	std::string codeStr;
	int code = 0;
	
	if (index_status == std::string::npos)
	{
		if (method == "POST")
			code = 201;
		else
			code = 200;
	}
	else
	{
		index_end_line = str.find("\n");
		if (index_end_line == std::string::npos)
			throw ServiceUnavailabledException();
		statusLine = str.substr(index_status, index_end_line);
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
	return code;
}