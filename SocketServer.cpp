#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "SocketServer.hpp"
#include <iostream>
#include <fstream>

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
	strlcpy(substr, s + start, len + 1);
	return (substr);
}

int	ft_memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned char	*p1;
	unsigned char	*p2;
	// int				subs;

	p1 = (unsigned char *)s1;
	p2 = (unsigned char *)s2;
	while (n > 0)
	{
		if (*p1 != *p2)
			return (-1);
		p1++;
		p2++;
		n--;
	}
	return (0);
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
	struct addrinfo hints, *serverInfo, *ptr; //struct info about server address
	int error;
	int opt = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //flag to set either iPV4 or iPV6
	hints.ai_socktype = SOCK_STREAM; //type of socket, we need TCP
	hints.ai_flags = AI_PASSIVE; //flag to set localhost as server address
	error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
	if (error == -1)
		std::cout << "Error getaddrinfo" << std::endl;
	//find first available socket
	for (ptr = serverInfo; ptr != NULL; ptr = ptr->ai_next)
	{
		_socketFd = socket(ptr->ai_family, ptr->ai_socktype, 0); //create socket
		if (_socketFd == -1)
		{
			printError(SOCKET);
			continue;
		}
		int status = fcntl(_socketFd, F_SETFL, O_NONBLOCK); //making socket non-blocking - not sure how to test it yet
		if (status == -1)
  			printError(FCNTL);
		if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //make the address reusable
		{
            printError(SETSOCKET);
            exit(1); //exit in cpp look up
        }
		if (bind(_socketFd, ptr->ai_addr, ptr->ai_addrlen) == -1) //binding address to port number
		{
			printError(BIND);
			continue;
		}
		break ;
	}
	freeaddrinfo(serverInfo);
	if (ptr == NULL)
	{
		perror("failed to connect to server");
		exit(1);
	}
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
						//add new fds in poll struct	
						client_pollfd.fd = _newSocketFd;
						client_pollfd.events = POLLIN;
						poll_sets.push_back(client_pollfd);

						/* to print IPversion 
						char s[256];
						//just checking wich IPV is - so i can print address properly
						if (their_addr.ss_family == AF_INET)
						{
							struct sockaddr_in *sin;
							sin = (struct sockaddr_in *)&their_addr;
							inet_ntop(their_addr.ss_family, sin, s, sizeof s);
							std::cout << "server: got connection from " << s << std::endl;
						}
						else if (their_addr.ss_family == AF_INET6)
						{
							struct sockaddr_in6 *sin;
							sin = (struct sockaddr_in6 *)&their_addr;
							inet_ntop(their_addr.ss_family, sin, s, sizeof s);
							std::cout << "server: got connection from " << s << std::endl;
						}
						else
							std::cout <<"Unknown" << std::endl;
						memset(&s, 0, sizeof(s)); */
					}
					else //old client sent something
					{
						//receives from client, if recv zero means client is not up anymore
						char buffer[BUFFER]; //for size, something enough big to hold client's message, but not big problem.
									  		// if there is no enough space, it sends message in more times
						// printf("buffer before recv %lu\n", sizeof(buffer));
						res = recv(it->fd, buffer, BUFFER - 1, 0);
						buffer[res] = '\0';
						if (res <= 0)
						{
							if (res == 0)
								std::cout << "socket number " << it->fd << " closed connection" << std::endl;
							else
								printError(RECEIVE);
							poll_sets.erase(it);
							close(it->fd);
							
						}
						else //data to write - server response to client message ~ to verify
						{
							//using strstr to find boundaries
							// if (it->fd & POLLOUT) //not sure its usage - it there is data to write //wrong place to be
							// {
								//parsing client message
								// printf("buffer %s\n", buffer);
								printf("bytes received %d\n", res);
								char *result = strstr(buffer, "=");
								// if (result != NULL)
								// 	printf("where I find = %s\n", result);
								result++;
								int index_start = result - buffer;
								int count = 0;
								while (1)
								{
									if (*result == '\r')
										break ;
									count++;
									result++;
								}
								printf("count %d\n", count);
								int i = 0;
								char *b;
								b = (char *)malloc(sizeof(char) * (count + 1));
								while (i < count - 1)
								{
									b[i] = buffer[index_start];
									i++;
									index_start++;
								}
								b[i] = '\0';
								// printf("boundry: %s, %ld\n", b, strlen(b));
								char *result2 = strstr(buffer, b);
								printf("i: %ld\n", result2 - buffer);
								printf("i: %s\n", ft_substr(buffer, result2 - buffer, strlen(b)));
								// strcat(b, "--");
								printf("boundry: %s\n", b);
								result2 = strstr(result2 + 1, b);
								printf("j: %ld\n", result2 - buffer);
								printf("j: %s\n", ft_substr(buffer, result2 - buffer, strlen(b) + 2));

								// printf("all: %s\n", ft_substr(buffer, 0, 143));
								// printf("all: %s\n", ft_substr(buffer, 0, 250));

								int blen = strlen(b);
								printf("boundary: %s, %d\n", b, blen);
								int last_index = 0;
								// strcat(b, "\r\n");
								// int index_start_last_b = res - blen - 2;
								// printf("b %s\n", b);
								for (int i = 0; i < res; i++) {
									int diff = ft_memcmp(buffer + i, b, blen);
									// printf("diff: %d")
									if (diff == 0) {
										printf("boundary index: %d\n", i);
										last_index = i;
									}
								}
								printf("k: %s\n", buffer + last_index);
								printf("last index %d\n", last_index);
								// printf("after boundary: %s\n", ft_substr(buffer, last_index + blen, 100));
								char new_line[] = "\r\n\r\n";
								int last_new_line = 0;
								for (int i = 0; i < 43176; i++) {
									int diff = ft_memcmp(buffer + i, new_line, 4);
									// printf("diff: %d")
									if (diff == 0) {
										printf("boundary index: %d\n", i);
										last_new_line = i;
										break ;
									}
								}
								printf("last_new_line: %d\n", last_new_line);
								int file = open("data.jpg", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
								if (file < 0) {
									perror("Error opening file");
									exit(1);
								}

								// Write the array to the file
								int header_size = last_new_line + 2;
								for (int i = header_size + 1; i < 43176; i++) {
									int diff = ft_memcmp(buffer + i, new_line, 4);
									// printf("diff: %d")
									if (diff == 0) {
										printf("boundary index: %d\n", i);
										last_new_line = i;
										break ;
									}
								}
								printf("header_size: %d\n", header_size);
								printf("last_new_line in section %d\n", last_new_line);
								ssize_t written = write(file, buffer + last_new_line + 4, res - header_size);
								if (written < 0) {
									perror("Error writing to file");
									close(file);
									exit(1);
								}
								close(file);
								// int size_binary = res - header_size;
								// printf("size binary is %d\n", size_binary);
								// index_start_last_b = res - blen;
								// printf("%s\n", buffer + index_start_last_b);
								//  for (int i = 0; i < size_binary; i++) {
								// 		char byte = body[i];
								// 		// Example: Print the byte in hexadecimal format
								// 		printf("%02x ", byte);
								//  }
								// char *body = buffer + last_new_line + 4;
								// printf("size body %lu\n", strlen(body));
								// for (int i = 0; i< size_binary; i++)
								// {
								// 	int diff = ft_memcmp(body, b, strlen(b));
								// 	if (diff == 0)
								// 		printf("b found at %d\n", diff);
								// }
								// Close the file						
								// result2 = strstr(result2 + 1, b);
								// printf("k: %ld\n", result2 - buffer);
								// if (result2 != NULL)
								// 	printf("reached the end %s\n", result2);
								// else
								// 	printf("result %s\n", result2);
								exit(1);
								// ServerParseRequest request;
								// std::map<std::string, std::string> infoRequest;
								// infoRequest = request.parseRequestHttp(buffer, info.getServerRootPath());
								// if (infoRequest.find("Method") != infoRequest.end()) //checks for methods we want to implement
								// {
								// 	ServerResponse serverResponse;
								// 	std::string response;
								// 	if (infoRequest["Method"] == "GET")
								// 	{
								// 		response = serverResponse.responseGetMethod(info, infoRequest);
								// 		if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
								// 			printError(SEND);
								// 		// std::cout << "Get response sent" << std::endl;
								// 	}
								// 	else if (infoRequest["Method"] == "POST")
								// 	{
								// 		std::string strBuff(buffer);
								// 		std::string boundary;
								// 		if (strBuff.find("=") != std::string::npos)
								// 		{
								// 			boundary = strBuff.substr(strBuff.find("=") + 1, strBuff.find("\r\n", strBuff.find("=")) - strBuff.find("=") + 1);
								// 		}
								// 		// std::cout << "boundary in sock " << boundary << std::endl;
								// 		// std::cout << hi << std::endl;
								// 		// serverResponse.responsePostMethod(info, infoRequest);
								// 		// std::cout << "handle 1POST" << std::endl;
								// 		response = serverResponse.responsePostMethod(info, infoRequest, buffer);
								// 		// std::cout << "handle 2POST" << std::endl;
								// 		// std::cout << response << std::endl;
								// 		if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
								// 			printError(SEND);
								// 		// std::cout << "handle 3POST" << std::endl;
								// 	}
								// 	else if (infoRequest["Method"] == "DELETE")
								// 	{
								// 		std::cout << "handle DELETE" << std::endl;
								// 	}
								// }
								// else
								// {
								// 	std::cout << "method not found" << std::endl;
								// }
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