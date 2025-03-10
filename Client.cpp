#include "Client.hpp"

// Constructor and destructor
Client::Client(int fd, InfoServer info)
{
    this->fd = fd;
    this->info = info;
}

// Getters
int     Client::getFd() const
{
	return this->fd;
}
ClientRequest Client::getRequest() const
{
	return this->request;
}

std::string  Client::getResponse() const
{
	return this->response;
}

// Main functions
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
	Logger::debug("tot bytes " + std::to_string(totBytes));
	if (result == DISCONNECTED)
		return 1;
	else if (result == NODATA)
	{
		Logger::debug("No data available");
        return 0;
	}
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
		std::cout << "here\n" << std::endl;
	}
	return 0;
}

// int Client::processClient(void)
// {
// 	std::string httpRequest =
// 	"POST /upload HTTP/1.1\r\n"
// 	"Host: example.com\r\n"
// 	"Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
// 	"Content-Length: 335\r\n"
// 	"Connection: keep-alive\r\n\r\n"
// 	"------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
// 	"Content-Disposition: form-data; name=\"file\"; filename=\"example.txt\"\r\n"
// 	"Content-Type: text/plain\r\n\r\n"
// 	"This is the content of the file.\r\n"
// 	"------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

// 	Logger::debug("before client request");
// 	ClientRequest request(httpRequest, httpRequest.length());
// 	this->request = request;
// 	if (isCgi(this->request.getRequestLine()["Request-URI"]) == true)
// 		return 0;
// 	else
// 	{
// 		this->response = prepareResponse(this->request);
// 		response.clear();
// 		Logger::info("Response created successfully and store in clientQueu");
// 	}
// 	return 0;
// }

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

std::ostream &operator<<(std::ostream &output, Client const &obj)
{
        output << "client fd: " << obj.getFd() << std::endl;
		output << "Client request\n";
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
		return output;
        output<< "response: " << obj.getResponse();
		return output;

}