#include "WebServer.hpp"
#include <ctime>
#define BUFFER 1024
#define MAX 1024

// Constructor and Destructor

Webserver::Webserver(Config& file)
{
	this->configInfo = file.getServList();
	int sizeList = this->configInfo.size();
	for (int i = 0; i < sizeList; i++)
	{
		std::string port = configInfo[i]->getPort();
		std::string ip = configInfo[i]->getIP();
		ServerSockets server(ip, port);
		this->serverFd = server.getServerSocket();
		this->configInfo[i]->setFD(this->serverFd);
	}
	this->poll_sets.reserve(MAX);
	for (int i = 0; i < sizeList; i++)
	{
		if (this->configInfo[i]->getFD() > 0)
			addServerSocketsToPoll(this->configInfo[i]->getFD());
	}
	std::string errorPagePath = "/www/errors"; //need to take care of this
	HttpException::setHtmlRootPath(errorPagePath);
}

Webserver::~Webserver()
{
	closeSockets();
}

// Setters and Getters

void Webserver::checkTime(void)
{
	std::time_t currentTime = std::time(NULL);
	std::vector<ClientHandler>::iterator clientIt;
	std::vector<struct pollfd>::iterator it;
	for (it = this->poll_sets.begin(); it < this->poll_sets.end(); it++)
	{
		clientIt = retrieveClient(it->fd);
		if (clientIt != this->clientsList.end())
		{
			if (currentTime - clientIt->getTime() > clientIt->getTimeOut())
			{
				Logger::error("Fd: " + Utils::toString(it->fd) + " timeout");
				removeClient(it);
			}
		}
	}
}
// Main functions

int	Webserver::startServer()
{
	int returnPoll;
	int sizeList = this->configInfo.size();

	for (int i = 0; i < sizeList; i++)
	{
		if (this->configInfo[i]->getFD() < 0)
		{
			Logger::error("Failed to create socket " + Utils::toString(this->configInfo[i]->getFD()) + " on port " + this->configInfo[i]->getPort());
			exit(1);
		}
	}
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
		checkTime();
	}
	return 0;
}

void Webserver::dispatchEvents()
{
	std::vector<struct pollfd>::iterator it;
	int result;

    for (it = poll_sets.begin(); it != poll_sets.end();) 
    {
		Logger::debug("Client: " + Utils::toString(it->fd));
		result = 0;
        if (it->revents & POLLIN)
        {
			if (fdIsServerSocket(it->fd) == true)
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
		status = clientIt->manageRequest();
	else
		status = clientIt->retrieveResponse();
	return status;
}

//Utils

int Webserver::fdIsServerSocket(int fd)
{
	int sizeList = this->configInfo.size();
	for (int i = 0; i < sizeList; i++)
	{
		if (this->configInfo[i]->getFD() == fd)
			return true;
	}
	return false;
}

InfoServer*	Webserver::matchFD( int fd ) {

	std::vector<InfoServer*>::iterator servIt;

	for(servIt = (*this).configInfo.begin(); servIt != (*this).configInfo.end(); servIt++)
	{
		if ((*servIt)->getFD() == fd)
			return (*servIt);
	}
	return NULL;
}

int Webserver::fdIsCGI(int fd)
{
	return false;
}

void Webserver::addServerSocketsToPoll(int fd)
{
    struct pollfd serverPoll[1];

	serverPoll[0].fd = fd;
	serverPoll[0].events = POLLIN;
	this->poll_sets.push_back(serverPoll[0]);
	Logger::info("Add server sockets to poll sets");
}

void Webserver::createNewClient(int fd)
{
	Logger::debug("New client on fd " + Utils::toString(fd));
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
	std::vector<pollfd> newSet = poll_sets;
	InfoServer *server = matchFD(fd);
	ClientHandler newClient(clientFd, *server);
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