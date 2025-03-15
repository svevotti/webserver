#include "ClientHandler.hpp"

// Constructor and destructor
ClientHandler::ClientHandler(int fd, InfoServer info)
{
    this->fd = fd;
    this->info = info;
}

// Getters
int     ClientHandler::getFd() const
{
	return this->fd;
}
ClientRequest ClientHandler::getRequest() const
{
	return this->request;
}

std::string  ClientHandler::getResponse() const
{
	return this->response;
}

// Main functions
int ClientHandler::readData(int fd, std::string &str, int &bytes)
{
	int res = 0;
	char buffer[BUFFER];

	while (1)
	{
		ft_memset(buffer, 0, sizeof(buffer));
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

int ClientHandler::receiveRequest(void)
{
	int result;
	std::string full_buffer;
	int totBytes = 0;

	result = readData(fd, full_buffer, totBytes);
	if (result == DISCONNECTED)
		return 0;
	else if (result == NODATA)
	{
		Logger::debug("No data available");
        return 1;
	}
	else
	{
        ClientRequest incomingRequest(full_buffer, totBytes);
		this->request = incomingRequest;
		Logger::info("Parsed completed");
		//TODO:check for malformed requests here
		if (isCgi(this->request.getRequestLine()["Request-URI"]) == true)
			return CGI;
		else
		{
			this->response = prepareResponse(this->request);
			Logger::info("Response created");
		}
	}
	return 2;
}

int ClientHandler::sendResponse()
{
	int bytes = send(this->fd, this->response.c_str(), 43168, MSG_DONTWAIT);
	Logger::debug("bytes sent " + std::to_string(bytes));
	if (bytes == -1)
		Logger::error("Failed send - Sveva check this out");
	this->response.clear();
	Logger::info("Response sent to " + std::to_string(this->fd));
	return bytes;
}

int	ClientHandler::searchPage(std::string path)
{
	FILE *folder;

	folder = fopen(path.c_str(), "rb");
	Logger::debug("path to file " + path);
	if (folder == NULL)
		Logger::error("Error opening file: " + std::string(strerror(errno)));
	fclose(folder);
	return true;
}

int ClientHandler::isCgi(std::string str)
{
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage(this->info.getServerRootPath() + str) == true)
		return false;
	return true;
}

std::string ClientHandler::prepareResponse(ClientRequest request)
{

	std::string response;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request.getRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		ServerResponse serverResponse(request, this->info);
		if (httpRequestLine["Method"] == "GET")
			response = serverResponse.responseGetMethod();
		else if (httpRequestLine["Method"] == "POST")
			response = serverResponse.responsePostMethod();
		else if (httpRequestLine["Method"] == "DELETE")
			response = serverResponse.responseDeleteMethod();
		else
			Logger::error("Method not found, Sveva, use correct status code line");
	}
	else
		Logger::error("Method not found, Sveva, use correct status code line");
	return response;
}

std::ostream &operator<<(std::ostream &output, ClientHandler const &obj)
{
        output << "client fd: " << obj.getFd() << std::endl;
		output << "ClientHandler request\n";
        std::map<std::string, std::string> map;
        std::map<std::string, std::string>::iterator it;
        map = obj.getRequest().getRequestLine();
        for (it = map.begin();  it != map.end(); it++)
        {
                output << it->first << " : " << it->second << std::endl;
        }
        output << "headers: \n";
        map.clear();
        map = obj.getRequest().getHeaders();
        for (it = map.begin();  it != map.end(); it++)
        {
                output << it->first << " : " << it->second << std::endl;
        }
        output<< "response: " << obj.getResponse();
		return output;

}