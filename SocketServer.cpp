#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "SocketServer.hpp"

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
						char hi[4096]; //for size, something enough big to hold client's message, but not big problem.
									  // if there is no enough space, it sends message in more times
						res = recv(it->fd, hi, sizeof(hi), 0);
						// printf("bytes recv %i\n", bytes);
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
							// if (it->fd & POLLOUT) //not sure its usage - it there is data to write //wrong place to be
							// {
								//parsing client message
								ServerParseRequest request;
								std::map<std::string, std::string> infoRequest;
								infoRequest = request.parseRequestHttp(hi, info.getServerRootPath());
								if (infoRequest.find("Method") != infoRequest.end()) //checks for methods we want to implement
								{
									ServerResponse serverResponse;
									std::string response;
									if (infoRequest["Method"] == "GET")
									{
										response = serverResponse.responseGetMethod(info, infoRequest);
										if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
											printError(SEND);
										std::cout << "Get response sent" << std::endl;
									}
									else if (infoRequest["Method"] == "POST")
									{
										// std::cout << hi << std::endl;
										// serverResponse.responsePostMethod(info, infoRequest);
										std::cout << "handle 1POST" << std::endl;
										response = serverResponse.responsePostMethod(info, infoRequest);
										std::cout << "handle 2POST" << std::endl;
										std::cout << response << std::endl;
										if (send(it->fd, response.c_str(), strlen(response.c_str()), 0) == -1)
											printError(SEND);
										std::cout << "handle 3POST" << std::endl;
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
								memset(&hi, 0, strlen(hi));
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