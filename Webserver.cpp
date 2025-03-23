#include "WebServer.hpp"
#define BUFFER 1024
#define MAX 100

// Constructor and Destructor

Webserver::Webserver(Config &file)
{
	this->infoServers = file.getServList();
	this->ipAddress = this->infoServers[0]->getIP();
	this->port = this->infoServers[0]->getPort();
	
	ServerSockets server(this->ipAddress, this->port);
	this->serverFd = server.getServerFd();
	this->poll_sets.reserve(MAX);
	if (this->serverFd > 0)
		addServerSocketsToPoll(this->serverFd);
}

Webserver::~Webserver()
{
	closeSockets();
}

// Setters and Getters

// Main functions
int	Webserver::startServer()
{
	int returnPoll;

	if (this->serverFd < 0)
		return -1;
	Logger::info("Start server: " + this->ipAddress + ", port: " + this->port);
	while (1)
	{
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 2 * 1000);
		if (returnPoll == -1)
		{
			Logger::error("Failed poll: " + std::string(strerror(errno)));
			break;
		}
		else if (returnPoll == 0)
			Logger::debug("Poll timeout: no events");
		else
			dispatchEvents();
	}
	return 0;
}

void Webserver::dispatchEvents()
{
	std::vector<struct pollfd>::iterator it;
	int result;

    for (it = poll_sets.begin(); it != poll_sets.end();) 
    {
		Logger::debug("FD: " + Utils::toString(it->fd));
		result = 0;
        if (it->revents & POLLIN)
        {

			if (it->fd == this->serverFd)
				createNewClient(it->fd);
			else if (fdIsCGI(it->fd) == true)
			{
				Logger::info("Start CGI logic here");
			}
			else
			{
				result = processClient(it->fd, READ);
				if (result == DISCONNECTED)
				{
					removeClient(it);
                    return ;
				}
				else if (result == 2)
					it->events = POLLOUT;
			}
        }
		else if (it->revents & POLLOUT)
		{
			if (processClient(it->fd, WRITE) == -1)
			{
				Logger::error("Something went wrong with send - don't know the meaning of it yet");
			}
			it->events = POLLIN;
		}
		else if (it->revents & POLLERR)
		{
			Logger::error("Fd " + Utils::toString(it->fd) + ": error");
			removeClient(it);
			return;
		}
		else if (it->revents & POLLHUP)
		{
			Logger::error("Fd " + Utils::toString(it->fd) + ": the other end has closed the connection.\n");
			removeClient(it);
			return;
		}
		else if (it->revents & POLLNVAL)
		{
			Logger::error("Fd " + Utils::toString(it->fd) + ": invalid request, fd not open");
			removeClient(it);
			return;
		}
		it++;
	}
}

int Webserver::processClient(int fd, int event)
{
	int status;
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClient(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return 0;
	}
	if (event == READ)
		status = clientIt->clientStatus();
	else
		status = clientIt->retrieveResponse();
	return status;
}

//Utils
// int Webserver::fdIsServerSocket(int fd)
// {
// 	int size = this->serverFds.size();
// 	for (int i = 0; i < size; i++)
// 	{
// 		if (fd == this->serverFds[i])
// 			return true;
// 	}
// 	return false;
// }

int Webserver::fdIsCGI(int fd)
{
	return false;
}

// void Webserver::addServerSocketsToPoll(std::vector<int> fds)
// {
//     struct pollfd serverPoll[MAX];
// 	int clientsNumber = (int)fds.size();
// 	if (clientsNumber == 0)
// 		return;
// 	for (int i = 0; i < clientsNumber; i++)
// 	{
// 		serverPoll[i].fd = fds[i];
// 		serverPoll[i].events = POLLIN;
// 		this->poll_sets.push_back(serverPoll[i]);
// 	}
// 	Logger::info("Add server sockets to poll sets");
// }

void Webserver::addServerSocketsToPoll(int fd)
{
    struct pollfd serverPoll[MAX];
	serverPoll[0].fd = fd;
	serverPoll[0].events = POLLIN;
	this->poll_sets.push_back(serverPoll[0]);
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
	ClientHandler newClient(clientFd, *this->infoServers[0]);
	this->clientsList.push_back(newClient);
	Logger::info("New client " + Utils::toString(newClient.getFd()) + " created and added to poll sets");
}

std::vector<ClientHandler>::iterator Webserver::retrieveClient(int fd)
{
	std::vector<ClientHandler>::iterator iterClient;
	std::vector<ClientHandler>::iterator endClient = this->clientsList.end();

	for (iterClient = this->clientsList.begin(); iterClient != endClient; iterClient++)
	{
		if (iterClient->getFd() == fd)
			return iterClient;
	}
	return endClient;
}

void Webserver::closeSockets()
{
	int size = (int)this->poll_sets.size();
	for (int i = 0; i < size; i++)
	{
		if(this->poll_sets[i].fd >= 0)
			close(this->poll_sets[i].fd);
	}
}

void Webserver::removeClient(std::vector<struct pollfd>::iterator it)
{
	Logger::info("Client " + Utils::toString(it->fd) + " disconnected");
	close(it->fd);
	this->clientsList.erase(retrieveClient(it->fd));
	this->poll_sets.erase(it);
}