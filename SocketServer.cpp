#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "SocketServer.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>

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

SocketServer::SocketServer(void)
{
	// std::cout << "Default constructor" << std::endl;
}

SocketServer::~SocketServer(void)
{
	// std::cout << "Destructor" << std::endl;
}

SocketServer::SocketServer(const SocketServer &other)
{
	_socketFd = other._socketFd;
	_newSocketFd = other._newSocketFd;
	_portNumber = other._portNumber;
	_serverAddr = other._serverAddr;
	_clientAddr = other._clientAddr;
	_lenClient = other._lenClient;
	// std::cout << "Copy constructor" << std::endl;
}

void	SocketServer::operator=(const SocketServer &other)
{
	_socketFd = other._socketFd;
	_newSocketFd = other._newSocketFd;
	_portNumber = other._portNumber;
	_serverAddr = other._serverAddr;
	_clientAddr = other._clientAddr;
	_lenClient = other._lenClient;
	// std::cout << "Copy assignment" << std::endl;
}

int createServerSocket(const char* portNumber)
{
	int _socketFd;
	struct addrinfo hints, *serverInfo; //struct info about server address
	int error;
	int opt = 1;

	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; //flag to set either iPV4 or iPV6
	hints.ai_socktype = SOCK_STREAM; //type of socket, we need TCP
	hints.ai_flags = AI_PASSIVE; //flag to set localhost as server address
	error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
	if (error == -1)
		std::cout << "Error getaddrinfo" << std::endl;
	//find first available socket
	// for (ptr = serverInfo; ptr != NULL; ptr = ptr->ai_next)
	// {
		_socketFd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0); //create socket
		if (_socketFd == -1)
		{
			printError(SOCKET);
			// continue;
		}
		if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //make the address reusable
		{
            printError(SETSOCKET);
            exit(1); //exit in cpp look up
        }
		int status = fcntl(_socketFd, F_SETFL, O_NONBLOCK); //making socket non-blocking - not sure how to test it yet
		if (status == -1)
  			printError(FCNTL);
		//void printFcntlFlag(_socketFd);
		if (bind(_socketFd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) //binding address to port number
		{
			printError(BIND);
			// continue;
		}
	// 	break ;
	// }
	freeaddrinfo(serverInfo);
	// if (ptr == NULL)
	// {
	// 	perror("failed to connect to server");
	// 	exit(1);
	// }
	//listen
	if (listen(_socketFd, 5) == -1) //make server socket listenning to incoming connections
	{
		printError(LISTEN);
		exit(1);
	}
	return (_socketFd);
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

void	SocketServer::startSocket(InfoServer info)
{
	struct sockaddr_storage their_addr; // struct to store client's address information
    socklen_t sin_size;
	std::vector<pollfd> poll_sets; //using vector to store fds in poll struct, it could have been an array
	struct pollfd myPoll[200]; //which size to give? there should be a max
	int arraySockets[2];
	int i;
	int res;
	std::vector<pollfd>::iterator it;
	std::vector<pollfd>::iterator end;
	pollfd client_pollfd;

	/*array of sockets*/
	for (i = 0; i < 2; i++)
	{
		arraySockets[i] = createServerSocket(info.getArrayPorts()[i].c_str());
		if (arraySockets[i] < 0)
			printError(arraySockets[i]);
	}
	/*fill in poll strcut with my sockets*/
	poll_sets.reserve(100);
	for (i = 0; i < 2; i++)
	{
		myPoll[i].fd = arraySockets[i];
		myPoll[i].events = POLLIN;
		poll_sets.push_back(myPoll[i]);
	}
	std::cout << "Waiting connections..." << std::endl;
	//inf loop to accept incoming connections
	while(1)
	{
		res = poll((pollfd *)&poll_sets[0], (unsigned int)poll_sets.size(), 1 * 60 * 1000);
        if (res == -1)
		{
            printError(POLL);
            break ;
        }
		else if (res == 0) //if there are no connections, it is just printing a message
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
					int indexFd = findMatchingSocket(it->fd, arraySockets); //look for socket in the array
					if (indexFd != -1) //new client to add
					{
						sin_size = sizeof(their_addr);
						_newSocketFd = accept(arraySockets[indexFd], (struct sockaddr *)&their_addr, &sin_size); //client socket
						if (_newSocketFd == -1)
						{
							printError(ACCEPT);
							continue;
						}
						//set non blocking socket
						int status = fcntl(_newSocketFd, F_SETFL, O_NONBLOCK); //making socket non-blocking - not sure how to test it yet
						if (status == -1)
							printError(FCNTL);
						//add new fds in poll struct
						client_pollfd.fd = _newSocketFd;
						client_pollfd.events = POLLIN;
						poll_sets.push_back(client_pollfd);
					}
					else //old client sent something
					{
						//receives from client, if recv zero means client is not up anymore
						//for buffer size, something enough big to hold client's message, but not big problem.
						// if there is no enough space, it sends message in more times
						//create a loop to store all bytes
						char *buffer;
						std::string full_buffer;
						int tot_bytes_recv = 0;
						//void printFcntlFlag(_newSocketFd);
						int contentLen = 0;
						// int i = 0;
						int findsizeonetime = 0;
						int size = BUFFER;
						while (1)
						{
							buffer = new char[size + 1];
							// ft_memset(&buffer, 0, strlen(buffer));
							res = recv(it->fd, buffer, size, 0);
							// printf("do i get seg fault\n");
							// printf("res in breaking statement %d\n", res);
							if (res <= 0)
							{
								delete[] buffer;
								printf("res in breaking statement %d\n", res);
								break;
							}
							buffer[res] = '\0';
							//can look for content lenght
							full_buffer.append(buffer, res); //need to specify how much you want to append...
							std::string content_str = "Content-Length: ";
							int len = content_str.size();
							if (full_buffer.find(content_str) != std::string::npos && findsizeonetime == 0)
							{
								if (full_buffer.find("\r\n", full_buffer.find(content_str)) != std::string::npos)
								{
									len = full_buffer.find("\r\n", full_buffer.find(content_str)) - full_buffer.find(content_str);
									std::string size_str = full_buffer.substr(full_buffer.find(content_str) + content_str.size(), len - content_str.size());
									std::stringstream ss;
									ss << size_str;
									ss >> contentLen;
									if (contentLen < size)
										size = contentLen;
								}
								findsizeonetime = 1;
							}
							tot_bytes_recv += res;
							// infoRecvLoop(i, res, buffer, full_buffer, contentLen, tot_bytes_recv);
							// i++;
							delete[] buffer;
						}
						// just to check why recv returns -1, can't use errno
						//check if buffer was free properly
						// std::cout << full_buffer << std::endl;
						// printf("tot butes received %d and full bytes %d\n", tot_bytes_recv, contentLen);
						if (tot_bytes_recv < contentLen && res <= 0)
						{
							if (res <= 0)
							{
								if (res == 0)
									std::cout << "socket number " << it->fd << " closed connection" << std::endl;
								else
								{
									switch (errno) {
										case EWOULDBLOCK:
											printf("No data available (non-blocking mode)\n");
											break;
										case ETIMEDOUT:
											printf("Receive operation timed out\n");
											break;
										case ECONNRESET:
											printf("Connection reset by peer\n");
											break;
										// Add more cases as needed
										default:
											printf("recv error: %s\n", strerror(errno));
											break;
									}
								}
									// printError(RECEIVE);
								poll_sets.erase(it);
								close(it->fd);
							}
						}
						else
						{
								ServerParseRequest request;
								std::map<std::string, std::string> infoRequest;
								infoRequest = request.parseRequestHttp(full_buffer.c_str(), info.getServerRootPath());
								if (infoRequest.find("Method") != infoRequest.end()) //checks for methods we want to implement
								{
									ServerResponse serverResponse;
									std::string response;
									if (infoRequest["Method"] == "GET")
									{
										response = serverResponse.responseGetMethod(info, infoRequest);
										if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
											printError(SEND);
										std::cout << "done with GET response" << std::endl;
									}
									else if (infoRequest["Method"] == "POST")
									{
										response = serverResponse.responsePostMethod(info, infoRequest, full_buffer.c_str(), tot_bytes_recv);
										if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
											printError(SEND);
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
								ft_memset(&buffer, 0, strlen(buffer));
							// }
						}
					}
						
				}
			}
		}
	}
	//closing clinets fds
	for (i = 0; i < 2; i++)
	{
		if(myPoll[i].fd >= 0)
			close(myPoll[i].fd);
	}
}