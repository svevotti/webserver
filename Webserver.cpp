#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "WebServer.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>
#include <dirent.h>

#include "PrintingFunctions.hpp"

#define RECEIVE -7
#define SEND -8
#define ACCEPT -9
#define POLL -10
#define BUFFER 1024
#define MAX 100

//Constructor and Destructor
Webserver::Webserver(InfoServer& info)
{
	this->_serverInfo = new InfoServer(info);
	ServerSockets serverFds(*this->_serverInfo);

	this->serverFds = serverFds.getServerSockets();
	this->totBytes = 0;
	this->full_buffer.clear();
	this->poll_sets.reserve(MAX);
	addServerSocketsToPoll(this->serverFds);
}

Webserver::~Webserver()
{
	delete this->_serverInfo;
	closeSockets();
}

//setters and getters
std::string Webserver::getFullBuffer(void) const
{
	return this->full_buffer;
}

//main functions
void	Webserver::startServer()
{
	int returnPoll;

	while (1)
	{
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
		if (returnPoll == -1)
		{
			printError(POLL);
			break;
		}
		else if (returnPoll == 0)
			std::cout << "No events\n";
		else
			dispatchEvents();
	}
}

void Webserver::dispatchEvents()
{
	end = this->poll_sets.end();
    for (this->it = poll_sets.begin(); this->it != end; this->it++) 
    {
        if (this->it->revents & POLLIN)
        {
			if (fdIsServerSocket(this->it->fd) == true)
				createNewClient(this->it->fd);
			else
				handleReadEvents(this->it->fd);
        }
		else if (this->it->revents & POLLOUT)
			handleWritingEvents(this->it->fd);
	}
}

void Webserver::handleWritingEvents(int fd)
{
	std::vector<struct client>::iterator iterClient;
	std::vector<struct client>::iterator endClient = this->clientsQueue.end();

	for (iterClient = this->clientsQueue.begin(); iterClient != endClient; it++)
	{
		if (iterClient->fd == fd)
		{
			if (send(fd, iterClient->response.c_str(), strlen(iterClient->response.c_str()), MSG_DONTWAIT) == -1)
				printError(SEND);
			this->it->events = POLLIN;
			iterClient->response.clear();
			clientsQueue.erase(iterClient);
			break;
		}
	}
}
void Webserver::handleReadEvents(int fd)
{
	std::string response;

	int contentLength = 0;
	int bytesRecv;
	int flag = 1;
	bytesRecv = readData(fd, this->full_buffer, this->totBytes);
	if (bytesRecv == 0)
	{
		std::cout << "socket number " << fd << " closed connection" << std::endl;
		close(fd);
		this->it = this->poll_sets.erase(this->it);
	}
	//TODO: check by deafult if the http headers is always sent:if the request is malformed, it could lead to problems, it does
	//TODO: errors
	//TODO: reorganize this function
	if (this->totBytes > 0)
	{
		if (this->full_buffer.find("Content-Length") == std::string::npos && this->full_buffer.find("POST") == std::string::npos)
			flag = 0;
		else if (this->full_buffer.find("Content-Length") != std::string::npos)
		{
			flag = 0;
			contentLength = atoi(this->full_buffer.substr(this->full_buffer.find("Content-Length") + 16).c_str());
		}
		if (this->totBytes > contentLength && flag == 0)
		{
			this->request = ParsingRequest(this->full_buffer, this->totBytes);
			if (isCgi(this->request->getRequestLine()["Request-URI"]) == true)
			{
				printf("send to CGI\n");
				//should return a string
				//adding in the infoC structure
			}
			else
			{
				response = prepareResponse(this->request);
				delete this->request;
				this->full_buffer.clear();
				this->totBytes = 0;
				struct client infoC;
				infoC.fd = fd;
				infoC.response = response;
				this->clientsQueue.push_back(infoC);
				this->it->events = POLLOUT;
			}
		}
	}
}

ClientRequest *Webserver::ParsingRequest(std::string str, int size)
{

	this->request = new ClientRequest();
	this->request->parseRequestHttp(str, size);
	return this->request;
}


std::string Webserver::prepareResponse(ClientRequest *request)
{
	std::string response;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request->getRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		ServerResponse serverResponse(*request, *this->_serverInfo);
		if (httpRequestLine["Method"] == "GET")
			response = serverResponse.responseGetMethod();
		else if (httpRequestLine["Method"] == "POST")
			response = serverResponse.responsePostMethod();
		else if (httpRequestLine["Method"] == "DELETE")
			response = serverResponse.responseDeleteMethod();
		else
			std::cout << "method not found" << std::endl;
	}
	else
		std::cout << "method not found" << std::endl;
	return response;
}

//utilis
int Webserver::fdIsServerSocket(int fd)
{
	int size = this->serverFds.size();
	for (int i = 0; i < size; i++)
	{
		if (fd == this->serverFds[i])
			return true;
	}
	return false;
}

void Webserver::addServerSocketsToPoll(std::vector<int> fds)
{
    struct pollfd serverPoll[MAX];
	int clientsNumber = (int)fds.size();
	for (int i = 0; i < clientsNumber; i++)
	{
		serverPoll[i].fd = fds[i];
		serverPoll[i].events = POLLIN;
		this->poll_sets.push_back(serverPoll[i]);
	}
}

int Webserver::createNewClient(int fd)
{
	socklen_t clientSize;
	struct sockaddr_storage clientStruct;
	struct pollfd clientPoll;

	clientSize = sizeof(clientStruct);
	fd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize);
	if (fd == -1)
		printError(ACCEPT);
	clientPoll.fd = fd;
	clientPoll.events = POLLIN;
	this->poll_sets.push_back(clientPoll);
	return (fd);
}

int Webserver::readData(int fd, std::string &str, int &bytes)
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
	return res;
}

int	Webserver::searchPage(std::string path)
{
	FILE *folder;

	folder = fopen(path.c_str(), "rb");
	if (folder == NULL)
		return false;
	fclose(folder);
	return true;
}

int Webserver::isCgi(std::string str)
{
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage(this->_serverInfo->getServerDocumentRoot() + str) == true)
		return false;
	return true;
}

void	Webserver::closeSockets()
{
	int size = (int)this->poll_sets.size();
	for (int i = 0; i < size; i++)
	{
		if(this->poll_sets[i].fd >= 0)
			close(this->poll_sets[i].fd);
	}
}