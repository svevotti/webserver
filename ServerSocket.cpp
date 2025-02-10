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
	_serverFds = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0); //create socket
	if (_serverFds == -1)
		printError(SOCKET);
	if (setsockopt(_serverFds, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //make the address reusable
		printError(SETSOCKET);
	int status = fcntl(_serverFds, F_SETFL, O_NONBLOCK); //making socket non-blocking
	if (status == -1)
		printError(FCNTL);
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

void serverParsingAndResponse(const char *str, InfoServer info, int fd, int size)
{
	std::cout << "Success reading - parsing" << std::endl;
	ServerParseRequest request;
	std::map<std::string, std::string> infoRequest;
	infoRequest = request.parseRequestHttp(str, info.getServerRootPath());
	std::map<std::string, std::string>::iterator element;
	std::map<std::string, std::string>::iterator ite = infoRequest.end();

	for(element = infoRequest.begin(); element != ite; element++)
	{
		std::cout << "parsed item\nkey: " << element->first << " - value: " <<element->second << std::endl;
	}
	std::cout << "Success reading - responding" << std::endl;
	if (infoRequest.find("Method") != infoRequest.end())
	{
		ServerResponse serverResponse;
		std::string response;
		if (infoRequest["Method"] == "GET")
		{
			response = serverResponse.responseGetMethod(info, infoRequest);
			if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1)
				printError(SEND);
			std::cout << "done with GET response" << std::endl;
		}
		else if (infoRequest["Method"] == "POST")
		{
			response = serverResponse.responsePostMethod(info, infoRequest, str, size);
			std::cout << "sending" << std::endl;
			if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1)
				printError(SEND);
			std::cout << "done with POST response" << std::endl;
		}
		else if (infoRequest["Method"] == "DELETE")
		{
			std::cout << "handle DELETE" << std::endl;
		}
	}
	else
	{
		std::cout << "method not found" << std::endl;
	}
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
	// int send_timeout = 2;
	// setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
	int status = fcntl(clientFd, F_SETFL, O_NONBLOCK); /*no need since it inherits from server socoet passed in accept*/
	if (status == -1)
		printError(FCNTL);
	return (clientFd);
}

int readData(int fd, std::string &str, int &bytes)
{
	int res;
	char buffer[BUFFER];

	while (1)
	{
		// std::cout << "first line loop" << std::endl;
		ft_memset(&buffer, 0,sizeof(buffer));
		res = recv(fd, buffer, BUFFER, 0);
		if (res <= 0)
			break;
		buffer[res] = '\0';
		str.append(buffer, res);
		bytes += res;
	}
	return res;
}

void	Server::startServer(InfoServer info)
{
	// struct sockaddr_storage their_addr; // struct to store client's address information
    // socklen_t sin_size;
	std::vector<pollfd> poll_sets; //using vector to store fds in poll struct, it could have been an array
	struct pollfd myPoll[200]; //which size to give? there should be a max - client max
	int arraySockets[2];
	int i;
	int res;
	int bytesRecv;
	std::vector<pollfd>::iterator it;
	std::vector<pollfd>::iterator end;
	int clientSocket;
	// pollfd client_pollfd;

	/*array of sockets - needed?*/
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
		std::cout << "Set poll" << std::endl;
		res = poll((pollfd *)&poll_sets[0], (unsigned int)poll_sets.size(), 1 * 60 * 1000);
        if (res == -1)
		{
            printError(POLL);
            break ;
        }
		else if (res == 0)
		{
			printf("Waiting connections timeout 1 minute...\n");
			break ;
		}
		else
		{
			end = poll_sets.end();
			for (it = poll_sets.begin(); it != end; it++)
			{
				if (it->revents & POLLIN) //if there is data to read
				{
					std::cout << "poll event POLLIN" << std::endl;
					std::cout << "Create new client" << std::endl;
					clientSocket = createClientSocket(it->fd);
					if (clientSocket == -1)
						continue;
					/*recv data*/
					std::string full_buffer;
					int totBytes = 0;
					std::cout << "Recv data from client" << std::endl;
					bytesRecv = readData(clientSocket, full_buffer, totBytes);
					if (bytesRecv == 0)
						std::cout << "socket number " << clientSocket << " closed connection" << std::endl;
					else if (bytesRecv == -1)
						res = printRecvFlag(errno);
					if (res == 1)
						serverParsingAndResponse(full_buffer.c_str(), info, clientSocket, totBytes);
					close(clientSocket);
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