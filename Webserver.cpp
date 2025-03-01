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

#define SOCKET -1
#define GETADDRINFO -2
#define FCNTL -3
#define SETSOCKET -4
#define BIND -5
#define LISTEN -6
#define RECEIVE -7
#define SEND -8
#define ACCEPT -9
#define POLL -10
#define BUFFER 1024

std::string serverParsingAndResponse(std::string str, InfoServer info, int fd, int size)
{
	std::cout << "Parsing" << std::endl;
	ClientRequest request;
	std::map<std::string, std::string> httpRequestLine;
	ServerResponse serverResponse;
	std::string response;

	request.parseRequestHttp(str, size);
	std::cout << "done parsing HTTP" << std::endl;
	httpRequestLine = request.getRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		if (httpRequestLine["Method"] == "GET")
		{
			response = serverResponse.responseGetMethod(info, request);
			if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1)
				printError(SEND);
			std::cout << "done with GET response" << std::endl;
		}
		else if (httpRequestLine["Method"] == "POST")
		{
			response = serverResponse.responsePostMethod(info, request, str, size);
			if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1)
				printError(SEND);
			std::cout << "done with POST response" << std::endl;
		}
		else if (httpRequestLine["Method"] == "DELETE")
		{
			response = serverResponse.responseDeleteMethod(info, request);
			if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1)
				printError(SEND);
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
	int bytesRecv;
	std::string response;

	std::cout << "Recv client request" << std::endl;
	bytesRecv = readData(fd, this->full_buffer, this->totBytes);
	if (bytesRecv == 0)
	{
		std::cout << "socket number " << fd << " closed connection" << std::endl;
		close(fd);
		this->it = this->poll_sets.erase(this->it);
	}
	if (this->totBytes > 0)
	{
		if (this->full_buffer.find("Content-Length") == std::string::npos)
		{
			response = serverParsingAndResponse(this->full_buffer, info, fd, this->totBytes);
			this->totBytes = 0;
			this->full_buffer.clear();
			close(fd);
			this->it = this->poll_sets.erase(this->it);
		}
		else
		{
			if (this->totBytes > atoi(this->full_buffer.substr(this->full_buffer.find("Content-Length") + 16).c_str()))
			{
				response = serverParsingAndResponse(this->full_buffer, info, fd, this->totBytes);
				this->totBytes = 0;
				this->full_buffer.clear();
				close(fd);
				this->it = this->poll_sets.erase(this->it);
			}
		}
	}
}

void Webserver::dispatchEvents(InfoServer server, std::vector<int> serverSockets)
{
	end = this->poll_sets.end();
    for (this->it = poll_sets.begin(); this->it != end; this->it++) 
    {
        if (this->it->revents & (POLLIN | POLLOUT)) 
        {
			if (this->it->revents & (POLLIN | POLLOUT)) 
			{
				if (this->it->fd == serverSockets[0] || it->fd == serverSockets[1])
					createNewClient(this->it->fd);
				// else if (cgi_map.find(it->fd) != cgi_map.end()) 
				// {
				//     CGITracker& tracker = cgi_map[it->fd];
				//     if (it->revents & POLLOUT && it->fd == tracker.cgi->getInPipeWriteFd())
				//         handleCGIInput(tracker, it);
				//     else if (it->revents & POLLIN && it->fd == tracker.cgi->getOutPipeReadFd())
				//         handleCGIOutput(tracker, it);
				// } 
				else if (it->revents & POLLIN)
					handleReadEvents(this->it->fd, server);
				// else if (this->it->revents & POLLOUT)
				// 	handleSendEvents(it);
        	}
		}
    }
}

int	Webserver::callPoll(InfoServer info, std::vector<int> serverFds)
{
	int returnPoll;

	std::cout << "Poll called" << std::endl;
	returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
	if (returnPoll == -1)
		printError(POLL);
	else if (returnPoll == 0)
		std::cout << "No events\n";
	else
		dispatchEvents(info, serverFds);
	return returnPoll;
}

void	Webserver::startServer(InfoServer info)
{
	ServerSockets serverFds(info);

	this->totBytes = 0;
	//std::cout << "before clearing: " << this->full_buffer << std::endl;
	this->full_buffer.clear(); //if i don't clean it, i get the path to the server root..?

	this->poll_sets.reserve(100); //why setting memory beforehand
	addServerSocketsToPoll(serverFds.getServerSockets());
	while (1)
	{
		if (callPoll(info, serverFds.getServerSockets()) == -1)
			break;
	}
	for (int i = 0; i < 2; i++)
	{
		if(poll_sets[i].fd >= 0)
			close(poll_sets[i].fd);
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