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

#define BUFFER 1024
#define MAX 100

//Constructor and Destructor
Webserver::Webserver(InfoServer& info)
{
	this->_serverInfo = new InfoServer(info);
	ServerSockets serverFds(*this->_serverInfo);

	if (this->serverFds.size() < 0)
		Logger::error("Failed creating any server socket");
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

//main functions
void	Webserver::startServer()
{
	int returnPoll;

	while (1)
	{
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
		if (returnPoll == -1)
		{
			Logger::error("Failed poll: " + std::string(strerror(errno)));
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
			//TODO:does it behave as recv?
			if (send(fd, iterClient->response.c_str(), strlen(iterClient->response.c_str()), MSG_DONTWAIT) == -1)
				Logger::error("Failed send");
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

	int contentLength = -1;
	int flag = 1;
	int bytesRecv;
	bytesRecv = readData(fd, this->full_buffer, this->totBytes);
	if (bytesRecv == 0)
	{
		Logger::info("Client " + std::to_string(fd) + " disconnected");
		close(fd);
		this->it = this->poll_sets.erase(this->it);
	}
	//TODO: check by deafult if the http headers is always sent:if the request is malformed, it could lead to problems, it does
	//TODO: right now we are ignoring -1 of recv
	//TODO: reorganize this function for keep it clean
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
				Logger::info("Response created successfully and store in clientQueu");
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
			Logger::error("Method not found, Sveva, use correct status code line");
	}
	else
		Logger::error("Method not found, Sveva, use correct status code line");
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
	Logger::info("Add server sockets to poll sets");
}

void Webserver::createNewClient(int fd)
{
	socklen_t clientSize;
	struct sockaddr_storage clientStruct;
	struct pollfd clientPoll;
	int	clientFd;

	clientSize = sizeof(clientStruct);
	clientFd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize);
	if (clientFd == -1)
	{
		Logger::error("Failed to create new client (accept): " + std::string(strerror(errno)));
		return;
	}
	clientPoll.fd = clientFd;
	clientPoll.events = POLLIN;
	this->poll_sets.push_back(clientPoll);
	Logger::info("New client " + std::to_string(clientFd) + " created and added to poll sets");
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