#include "Client.hpp"

Client::Client(int fd, InfoServer info)
{
    this->fd = fd;
    this->info = info;
}

int Client::readData(int fd, std::string &str, int &bytes)
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

int Client::processClient(void)
{
	int result;
	std::string full_buffer;
	int totBytes = 0;

	result = readData(fd, full_buffer, totBytes);
	if (result == DISCONNECTED)
		return 1;
	else if (result == NODATA)
        return 0;
	else
	{
        ClientRequest request(full_buffer, totBytes); //pointer or not?
		this->request = request;
		if (isCgi(this->request.getRequestLine()["Request-URI"]) == true)
			return CGI;
		else
		{
			this->response = prepareResponse(this->request);
			response.clear();
			full_buffer.clear();
			totBytes = 0;
			Logger::info("Response created successfully and store in clientQueu");
		}
	}
	return;
}

int	Client::searchPage(std::string path)
{
	FILE *folder;

	folder = fopen(path.c_str(), "rb");
	if (folder == NULL)
		return false;
	fclose(folder);
	return true;
}

int Client::isCgi(std::string str)
{
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage(this->info.getServerDocumentRoot() + str) == true)
		return false;
	return true;
}

std::string Client::prepareResponse(ClientRequest request)
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