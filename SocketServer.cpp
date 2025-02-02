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
#define BUFFER 1000000

void printIpvVersion(struct sockaddr_storage address)
{
	char s[256];
						//just checking wich IPV is - so i can print address properly
	if (address.ss_family == AF_INET)
	{
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)&address;
		inet_ntop(address.ss_family, sin, s, sizeof(s));
		std::cout << "server: got connection from " << s << std::endl;
	}
	else if (address.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *sin;
		sin = (struct sockaddr_in6 *)&address;
		inet_ntop(address.ss_family, sin, s, sizeof(s));
		std::cout << "server: got connection from " << s << std::endl;
	}
	else
		std::cout <<"Unknown" << std::endl;
	memset(&s, 0, sizeof(s));
}

char	*ft_substr(char const *s, unsigned int start, size_t len)
{
	char	*substr;
	size_t	substr_len;

	if ((size_t)start > strlen(s))
	{
		substr = (char *)malloc(sizeof(char) * (1));
		if (substr == NULL)
			return (NULL);
		substr[0] = '\0';
		return (substr);
	}
	substr_len = strlen(s + start);
	if (len > substr_len)
		len = substr_len;
	substr = (char *)malloc(sizeof(char) * (len + 1));
	if (substr == NULL)
		return (NULL);
	strncpy(substr, s + start, len + 1);
	return (substr);
}

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

void printError(int error)
{
	switch (error)
	{
		case -1:
		std::cerr << "Error creating socket" << std::endl;
		break ;
		case -2:
		std::cerr << "Error getaddrinfo" << std::endl;
		break ;
		case -3:
		std::cerr << "Error fcntl" << std::endl;
		break ;
		case -4:
		std::cerr << "Error setsocket" << std::endl;
		break ;
		case -5:
		std::cerr << "Error bind" << std::endl;
		break ;
		case -6:
		std::cerr << "Error listen" << std::endl;
		break;
		case -7:
		std::cerr << "Error receive" << std::endl;
		break;
		case -8:
		std::cerr << "Error send" << std::endl;
		break;
		default:
		case -9:
		std::cerr << "Error accept" << std::endl;
		break;
		case -10:
		std::cerr << "Error poll" << std::endl;
		break;
		std::cerr << "Some other error" << std::endl;
	}
}

int createServerSocket(const char* portNumber)
{
	int _socketFd;
	struct addrinfo hints, *serverInfo; //struct info about server address
	int error;
	int opt = 1;

	memset(&hints, 0, sizeof(hints));
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
		else 
		{
    // Check if the socket is in non-blocking mode
			int flags = fcntl(_socketFd, F_GETFL); // Get the current flags
			if (flags & O_NONBLOCK)
				printf("Socket correctly set to non-blocking\n");
			else 
				printf("Socket is not set to non-blocking\n");
		}
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
	// int l = 0;

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
						//set non blocking
						int status = fcntl(_newSocketFd, F_SETFL, O_NONBLOCK); //making socket non-blocking - not sure how to test it yet
						if (status == -1)
							printError(FCNTL);
					// 	else 
					// 	{
					// // Check if the socket is in non-blocking mode
					// 		int flags = fcntl(_socketFd, F_GETFL); // Get the current flags
					// 		if (flags & O_NONBLOCK)
					// 			printf("Socket correctly set to non-blocking\n");
					// 		else 
					// 			printf("Socket is not set to non-blocking\n");
					// 	}
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
						/*char buffer[BUFFER];
						res = recv(it->fd, buffer, BUFFER - 1, 0);
						buffer[res] = '\0';
						if (res <= 0)
						{
							printf("here\n");
							if (res == 0)
								std::cout << "socket number " << it->fd << " closed connection" << std::endl;
							else
								printError(RECEIVE);
							poll_sets.erase(it);
							close(it->fd);
						*/
						//create a loop to store all bytes
						char buffer[BUFFER];
						std::string full_buffer;
						int tot_bytes_recv = 0;
						int flag = fcntl(it->fd, F_GETFL);
						if (flag & O_NONBLOCK)
							std::cout << "Socket number " << it->fd << " is non-blocking" << std::endl;
						else
							std::cout << "Socket number " << it->fd << " is blocking" << std::endl;
						int contentLen = 0;
						int i = 0;
						while (1)
						{
							res = recv(it->fd, buffer, BUFFER - 1, 0);
							if (res <= 0)
								break ;
							printf("res %d\n", res);
							buffer[res] = '\0';
							//can look for content lenght
							// printf("headers %s\n", buffer);
							full_buffer.append(buffer);
							std::cout << "loop: " << i << std::endl;
							std::cout << "buffer: " << full_buffer << std::endl;
							std::string content_str = "Content-Length: ";
							int len = content_str.size();
							// std::cout << "len of line is: " << len << std::endl;
							if (full_buffer.find(content_str) != std::string::npos)
							{
								// printf("index starting line %ld\n", full_buffer.find(content_str));
								if (full_buffer.find("\r\n", full_buffer.find(content_str)) != std::string::npos)
								{
									// printf("index end line %ld\n", full_buffer.find("\r\n", full_buffer.find(content_str)));
									len = full_buffer.find("\r\n", full_buffer.find(content_str)) - full_buffer.find(content_str);
									// std::string sub = full_buffer.substr(full_buffer.find(content_str), len);
									// std::cout << "substr and length: ..." << len << " " << sub << "..." << std::endl;
									std::string size_str = full_buffer.substr(full_buffer.find(content_str) + content_str.size(), len - content_str.size());
									// std::cout << "start: " << full_buffer.find(content_str) + content_str.size() << std::endl;
									// std::cout << "should be number: " << full_buffer[full_buffer.find(content_str) + content_str.size()] << std::endl;
									std::stringstream ss;
									ss << size_str;
									ss >> contentLen;
								}
								std::cout << "tot length: " << contentLen << std::endl;
							}
							tot_bytes_recv += res;
							printf("adding to tot bytes %d\n", tot_bytes_recv);
							i++;
						}
						printf("after loop tot bytes %d\n", tot_bytes_recv);
						printf("after loop res %d\n", res);
						if (tot_bytes_recv < contentLen || res == 0)
						{
							if (res <= 0)
							{
								if (res == 0)
									std::cout << "socket number " << it->fd << " closed connection" << std::endl;
								else
									printError(RECEIVE);
								// else if (tot_bytes_recv == -1)
								// 	printError(RECEIVE);
								poll_sets.erase(it);
								close(it->fd);
							}
						}
						else //if ((res & EAGAIN) || (res & EWOULDBLOCK)) it means it is in non blocking mode and no more data //data to write - server response to client message ~ to verify
						{
							// char *result = strstr(buffer, "POST");
							// if (result != NULL)
							// {
							// 	res = recv(it->fd, buffer + res, BUFFER - 1, 0);
							// 	if (res == 0)
							// 		std::cout << "socket number " << it->fd << " closed connection" << std::endl;
							// 	else if (res < 0)
							// 		printError(RECEIVE);
							// 	buffer[res] = '\0';
							// }
							//using strstr to find boundaries
							// if (it->fd & POLLOUT) //not sure its usage - it there is data to write //wrong place to be
							// {
								//parsing client message
								printf("bytes received %ld\n", full_buffer.length());
								std::cout << "full buffer: " << full_buffer << std::endl;
								// std::string headers(buffer);
								// std::cout << l << " : " << headers << std::endl;
								// l++;
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
										// std::cout << "Get response sent" << std::endl;
									}
									else if (infoRequest["Method"] == "POST")
									{
										response = serverResponse.responsePostMethod(info, infoRequest, full_buffer.c_str(), tot_bytes_recv);
										if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
											printError(SEND);
										// std::cout << "handle 3POST" << std::endl;
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
								memset(&buffer, 0, strlen(buffer));
							// }
						}
					}
						
				}
			}
		}
	}
	//closinf fds
	for (i = 0; i < 2; i++)
	{
		if(myPoll[i].fd >= 0)
			close(myPoll[i].fd);
	}
}