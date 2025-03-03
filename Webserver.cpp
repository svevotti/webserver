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

#include "PrintingFunctions.hpp"

#define RECEIVE -7
#define SEND -8
#define ACCEPT -9
#define POLL -10
#define BUFFER 1024

Webserver::Webserver(InfoServer info)
{
	ServerSockets serverFds(info);

	this->serverFds = serverFds.getServerSockets();
	this->totBytes = 0;
	this->full_buffer.clear();
	this->poll_sets.reserve(100); //why setting memory beforehand
	addServerSocketsToPoll(this->serverFds);
}
std::string prepareResponse(ClientRequest *request, InfoServer info)
{
	std::cout << "Response" << std::endl;
	std::string response;
	ServerResponse serverResponse;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request->getRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		ClientRequest lazyRequest(*request);
		if (httpRequestLine["Method"] == "GET")
		{
			response = serverResponse.responseGetMethod(info, lazyRequest);
			std::cout << "done with GET response" << std::endl;
		}
		else if (httpRequestLine["Method"] == "POST")
		{
			response = serverResponse.responsePostMethod(info, lazyRequest);
			std::cout << "done with POST response" << std::endl;
		}
		else if (httpRequestLine["Method"] == "DELETE")
		{
			response = serverResponse.responseDeleteMethod(info, lazyRequest);
			std::cout << "done with DELETE response" << std::endl;
		}
		else
		std::cout << "method not found" << std::endl;
	}
	else
	{
		std::cout << "method not found" << std::endl;
	}
	return response;
}

int isStatic(std::string str)
{
	if (str.find(".py") == std::string::npos)
		return true;
	return false;
}

ClientRequest *Webserver::ParsingRequest(std::string str, int size)
{
	std::string uri;
	std::string method;

	std::cout << "Parsing" << std::endl;
	this->request = new ClientRequest();
	this->request->parseRequestHttp(str, size);
	return this->request;
}

int Webserver::createNewClient(int fd)
{
	socklen_t clientSize;
	struct sockaddr_storage clientStruct;
	struct pollfd clientPoll;

	clientSize = sizeof(clientStruct);
	fd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize); //client socket
	if (fd == -1)
		printError(ACCEPT);
	clientPoll.fd = fd;
	clientPoll.events = POLLIN;
	this->poll_sets.push_back(clientPoll);
	//clean struct pollfd?
	std::cout << "client created: " <<  fd << std::endl;
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
		{
			break;
		}
		str.append(buffer, res);
		bytes += res;
	}
	return res;
}

void Webserver::handleReadEvents(int fd, InfoServer info)
{
	std::string response;

	int contentLength = -1;
	int bytesRecv;
	int flag = -1;
	std::cout << "handle read events\n";
	bytesRecv = readData(fd, this->full_buffer, this->totBytes);
	printf("tot bytes %d\n", this->totBytes);
	if (bytesRecv == 0)
	{
		std::cout << "socket number " << fd << " closed connection" << std::endl;
		close(fd);
		this->it = this->poll_sets.erase(this->it);
	}
	//TODO: check by deafult if the http headers is always sent
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
			response = prepareResponse(this->request, info);
			delete this->request;
			this->full_buffer.clear();
			this->totBytes = 0;
			struct client info;
			info.fd = fd;
			info.response = response;
			this->clientsQueue.push_back(info);
			this->it->events = POLLOUT;
		}
	}
	printf("after check bytes\n");
}

void Webserver::dispatchEvents(InfoServer server, std::vector<int> serverSockets)
{
	end = this->poll_sets.end();
    for (this->it = poll_sets.begin(); this->it != end; this->it++) 
    {
        if (this->it->revents & POLLIN)
        {
			std::cout << "pollin" << std::endl;
			//TODO not fix array of server sockets
			if (this->it->fd == serverSockets[0] || it->fd == serverSockets[1])
				createNewClient(this->it->fd);
			else
				handleReadEvents(this->it->fd, server);
        }
		else if (this->it->revents & POLLOUT)
		{
			std::cout << "pollout" << std::endl;
			std::vector<struct client>::iterator iterClient;
			std::vector<struct client>::iterator endClient = this->clientsQueue.end();
			for (iterClient = this->clientsQueue.begin(); iterClient != endClient; it++)
			{
				if (iterClient->fd == this->it->fd)
				{
					send(this->it->fd, iterClient->response.c_str(), strlen(iterClient->response.c_str()), 0);
					this->it->events = POLLIN;
					iterClient->response.clear();
					clientsQueue.erase(iterClient);
					break;
				}
			}
		}
	}
}

void	Webserver::startServer(InfoServer info)
{
	int returnPoll;

	while (1)
	{
		std::cout << "Poll called" << std::endl;
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
		if (returnPoll == -1)
		{
			printError(POLL);
			break;
		}
		else if (returnPoll == 0)
			std::cout << "No events\n";
		else
			dispatchEvents(info, serverFds);
	}
}

void Webserver::addServerSocketsToPoll(std::vector<int> fds)
{
    struct pollfd serverPoll[200];
	for (int i = 0; i < 2; i++)
	{
		serverPoll[i].fd = fds[i];
		serverPoll[i].events = POLLIN;
		this->poll_sets.push_back(serverPoll[i]);
	}
	//clean struct pollfd?
}

void	Webserver::closeSockets()
{
	for (int i = 0; i < 2; i++)
	{
		if(this->poll_sets[i].fd >= 0)
			close(this->poll_sets[i].fd);
	}
}

std::string Webserver::getFullBuffer(void) const
{
	return this->full_buffer;
}