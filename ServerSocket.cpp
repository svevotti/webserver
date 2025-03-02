#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "ServerSocket.hpp"
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

int createServerSocket(const char* portNumber)
{
	int _serverFds;
	struct addrinfo hints, *serverInfo; //struct info about server address
	int error;
	int opt = 1;

	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; //flag to set iPV6
	hints.ai_socktype = SOCK_STREAM; //type of socket, we need TCP, stream socket
	hints.ai_flags = AI_PASSIVE; //flag to set localhost as server address
	error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
	if (error == -1)
		std::cout << "Error getaddrinfo" << std::endl;
	/*note: loop to check socket availabilyt or not*/
	_serverFds = socket(serverInfo->ai_family, serverInfo->ai_socktype | SOCK_NONBLOCK, 0); //create socket
	if (_serverFds == -1)
		printError(SOCKET);
	if (setsockopt(_serverFds, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //make the address reusable
		printError(SETSOCKET);
	if (bind(_serverFds, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) //binding address to port number
		printError(BIND);
	/*till here the loop*/
	freeaddrinfo(serverInfo);
	if (listen(_serverFds, 5) == -1) //make server socket listenning to incoming connections
		printError(LISTEN);
	return (_serverFds);
}

int findMatchingSocket(int pollFd, int array[])
{
	int i = 0;
	for (i = 0; i < 2; i++)
	{
		if (pollFd == array[i])
			return(i);
	}
	return -1;
}

std::string serverParsingAndResponse(std::string str, InfoServer info, int fd, int size)
{
	std::cout << "Parsing" << std::endl;
	ClientRequest request;
	std::map<std::string, std::string> httpRequestLine;
	ServerResponse serverResponse;
	std::string response;

	request.parseRequestHttp(str, size);
	std::cout << "done parsing HTTP" << std::endl;
	httpRequestLine = request.getHttpRequestLine();
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

int getRequestContentLength(std::string buffer)
{
	std::string content_str = "Content-Length: ";
	int len = content_str.size();
	int contentLength = 0;
	if (buffer.find(content_str) != std::string::npos)
	{
		if (buffer.find("\r\n", buffer.find(content_str)) != std::string::npos)
		{
			len = buffer.find("\r\n", buffer.find(content_str)) - buffer.find(content_str);
			std::string size_str = buffer.substr(buffer.find(content_str) + content_str.size(), len - content_str.size());
			std::stringstream ss;
			ss << size_str;
			ss >> contentLength;
			return contentLength;
		}
	}
	return -1;
}

int createClientSocket(int fd)
{
	int clientFd;
	socklen_t clientSize;
	struct sockaddr_storage clientStruct;

	clientSize = sizeof(clientStruct);
	clientFd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize); //client socket
	if (clientFd == -1)
		printError(ACCEPT);
	return (clientFd);
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

void	Server::startServer(InfoServer info)
{

	std::vector<struct pollfd> poll_sets; //using vector to store fds in poll struct, it could have been an array
	struct pollfd myPoll[200]; //which size to give? there should be a max - client max
	int arraySockets[2];
	int i;
	int returnPoll;
	int bytesRecv;
	std::vector<pollfd>::iterator it;
	std::vector<pollfd>::iterator end;
	int clientSocket;
	std::string full_buffer;
	std::string response;
	int totBytes = 0;
	struct client client_info;

	for (i = 0; i < 2; i++)
	{
		arraySockets[i] = createServerSocket(info.getArrayPorts()[i].c_str());
		if (arraySockets[i] < 0)
			printError(arraySockets[i]);
	}
	/*fill in poll strcut with my server sockets*/
	poll_sets.reserve(100);
	for (i = 0; i < 2; i++)
	{
		myPoll[i].fd = arraySockets[i];
		myPoll[i].events = POLLIN;
		poll_sets.push_back(myPoll[i]);
	}
	std::cout << "Waiting connections..." << std::endl;
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
		{
			printf("Waiting connections\n");
			continue;
		}
		else
		{
			end = poll_sets.end();
			for (it = poll_sets.begin(); it != end; it++)
			{
				if (it->revents & POLLIN)
				{
					std::cout << "poll event POLLIN on fd: " << it->fd << std::endl;
					if (it->fd == arraySockets[0] || it->fd == arraySockets[1])
					{
						clientSocket = createClientSocket(it->fd);
						if (clientSocket == -1)
							continue;
						client_info.fds = clientSocket;
						client_info.bytes = 0;
						struct pollfd clientFd;
						clientFd.fd = clientSocket;
						clientFd.events = POLLIN;
						poll_sets.push_back(clientFd);
						std::cout << "client created: " <<  clientSocket << std::endl;
					}
					else
					{
						/*recv data*/
						std::cout << "Recv client request" << std::endl;
						bytesRecv = readData(it->fd, full_buffer, totBytes);
						if (bytesRecv == 0)
						{
							std::cout << "socket number " << it->fd << " closed connection" << std::endl;
							close(it->fd);
							it = poll_sets.erase(it);
						}
						if (totBytes > 0)
						{
							if (full_buffer.find("Content-Length") == std::string::npos)
							{
								response = serverParsingAndResponse(full_buffer, info, it->fd, totBytes);
								totBytes = 0;
								full_buffer.clear();
								close(it->fd);
								it = poll_sets.erase(it);
							}
							else
							{
								if (totBytes > atoi(full_buffer.substr(full_buffer.find("Content-Length") + 16).c_str()))
								{
									response = serverParsingAndResponse(full_buffer, info, it->fd, totBytes);
									totBytes = 0;
									full_buffer.clear();
									close(it->fd);
									it = poll_sets.erase(it);
								}
							}
						}
					}
				}
			}
		}
	}
	for (i = 0; i < 2; i++)
	{
		if(myPoll[i].fd >= 0)
			close(myPoll[i].fd);
	}
}