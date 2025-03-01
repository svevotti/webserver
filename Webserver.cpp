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

typedef struct client
{
	int fds;
	std::string buffer;
	int bytes;
} client;

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

int readData(int fd, std::string &str, int &bytes)
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

void	Webserver::startServer(InfoServer info)
{
	ServerSockets serverFds(info);

	int returnPoll;
	int bytesRecv;
	std::string full_buffer;
	std::string response;
	int totBytes = 0;
	std::vector<struct pollfd>::iterator it;
	std::vector<struct pollfd>::iterator end;

	this->poll_sets.reserve(100); //why setting memory beforehand
	addServerSocketsToPoll(serverFds.getServerSockets());
	while(1)
	{
		std::cout << "Poll called" << std::endl;
		returnPoll = poll(poll_sets.data(), poll_sets.size(), 1 * 20 * 1000);
        if (returnPoll == -1)
		{
            printError(POLL);
            break ;
        }
		else if (returnPoll == 0)
			continue;
		else
		{
			//dispatchEvents(info, serverFds.getServerSockets());
			end = this->poll_sets.end();
			for (it = this->poll_sets.begin(); it != end; it++)
			{
				if (it->revents & POLLIN)
				{
					std::cout << "poll event POLLIN on fd: " << it->fd << std::endl;
					if (it->fd == serverFds.getServerSockets()[0] || it->fd == serverFds.getServerSockets()[1])
						createNewClient(it->fd);
					else
					{
						/*recv data*/
						std::cout << "Recv client request" << std::endl;
						bytesRecv = readData(it->fd, full_buffer, totBytes);
						if (bytesRecv == 0)
						{
							std::cout << "socket number " << it->fd << " closed connection" << std::endl;
							close(it->fd);
							it = this->poll_sets.erase(it);
						}
						if (totBytes > 0)
						{
							if (full_buffer.find("Content-Length") == std::string::npos)
							{
								response = serverParsingAndResponse(full_buffer, info, it->fd, totBytes);
								totBytes = 0;
								full_buffer.clear();
								close(it->fd);
								it = this->poll_sets.erase(it);
							}
							else
							{
								if (totBytes > atoi(full_buffer.substr(full_buffer.find("Content-Length") + 16).c_str()))
								{
									response = serverParsingAndResponse(full_buffer, info, it->fd, totBytes);
									totBytes = 0;
									full_buffer.clear();
									close(it->fd);
									it = this->poll_sets.erase(it);
								}
							}
						}
					}
				}
			}
		}
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