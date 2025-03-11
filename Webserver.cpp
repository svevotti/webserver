#include "WebServer.hpp"
#define BUFFER 1024
#define MAX 100

// Constructor and Destructor
Webserver::Webserver(InfoServer info)
{
	this->serverInfo = info;
	ServerSockets fds(this->serverInfo);

	this->serverFds = fds.getServerSockets();
	this->poll_sets.reserve(MAX);
	addServerSocketsToPoll(this->serverFds);
}

Webserver::~Webserver()
{
	closeSockets();
}

// Setters and getters
// Main functions
int	Webserver::startServer()
{
	int returnPoll;

	if (this->serverFds.size() == 0)
		return -1;
	while (1)
	{
		Logger::info("Poll is monotoring");
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), TIMEOUT);
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

    for (it = poll_sets.begin(); it != this->poll_sets.end();) 
    {
		Logger::debug("fd " + std::to_string(it->fd));
        if (it->revents & POLLIN)
        {
			if (fdIsServerSocket(it->fd) == true)
				createNewClient(it->fd);
			else if (fdIsCGI(it->fd) == true)
			{
				printf("start CGI here\n");
			}
			else
			{
				result = processClient(it);
				if (result == DISCONNECTED)
					return;
				else if (result == CGI)
					printf("set up CGI here\n");
				else
					it->events = POLLOUT;
			}
        }
		else if (it->revents & POLLOUT)
		{
			result = responseForClient(it->fd);
			if (result == -1)
				Logger::error("Some error to define has being occured - review");
			it->events = POLLIN;
		}
		it++;
	}
}

int Webserver::fdIsCGI(int fd)
{
	return false;
}


// Utils
int	Webserver::processClient(std::vector<struct pollfd>::iterator it)
{
	int i;
	int result = 0;

	i = retrieveClientIndex(it->fd);
	if (i != -1)
		result = this->clientList[i].receiveRequest();
	else
		Logger::error("Client not found - bad news - maybe review");
	if (result == DISCONNECTED)
	{
		Logger::info("Client " + std::to_string(it->fd) + " disconnected");
		close(it->fd);
		i = retrieveClientIndex(it->fd);
		this->clientList.erase(this->clientList.begin() + i);
		this->poll_sets.erase(it);
		return result;
	}
	Logger::debug("path to image: " + this->clientList[i].getRequest().getRequestLine()["Request-URI"]);
	Logger::info("Client received the full request");
	return result;
}

int Webserver::responseForClient(int fd)
{
	int i;
	int result = -1;

	i = retrieveClientIndex(fd);
	if (i != -1)
		result = this->clientList[i].sendResponse();
	else
		Logger::error("Client not found to send to - bad news - please review");
	return result;
}

int Webserver::fdIsServerSocket(int fd)
{
	int size = this->serverFds.size();
	for (int i = 0; i < size; i++)
	{
		if (fd == this->serverFds[i])
			return true;
	}
	return false;
}

void Webserver::addServerSocketsToPoll(std::vector<int> fds)
{
    struct pollfd serverPoll[MAX];
	int clientsNumber = (int)fds.size();
	for (int i = 0; i < clientsNumber; i++)
	{
		serverPoll[i].fd = fds[i];
		serverPoll[i].events = POLLIN;
		this->poll_sets.push_back(serverPoll[i]);
		Logger::info("Add server socket " + std::to_string(this->serverFds[i]) + " to poll sets");
	}
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
	ClientHandler client(clientFd, this->serverInfo);
	this->clientList.push_back(client);
	Logger::info("New client " + std::to_string(clientFd) + " created and added to poll sets");
}

int Webserver::retrieveClientIndex(int fd)
{
	for (int i = 0; i < this->clientList.size(); i++)
	{
		if (this->clientList[i].getFd() == fd)
			return i;
	}
	return -1;
}

void	Webserver::closeSockets()
{
	int size = (int)this->poll_sets.size();
	for (int i = 0; i < size; i++)
	{
		if(this->poll_sets[i].fd >= 0)
			close(this->poll_sets[i].fd);
	}
}