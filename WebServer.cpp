#include "WebServer.hpp"
#include <ctime>
#include <signal.h>
#define BUFFER 1024
#define MAX 1024
#define DONE 2

// Constructor and Destructor
typedef struct m_pid
{
	int cgi_fd;
	int clent_fd;
} t_pid;

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
}

Webserver::~Webserver()
{
	closeSockets();
}

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
		std::cout << "exit dispatch\n";
		checkTime();
		std::cout << "after timecheck\n";
	}
	return 0;
}

void Webserver::dispatchEvents()
{
	std::vector<struct pollfd>::iterator it;
	int result = -2;

    for (it = poll_sets.begin(); it != poll_sets.end();)
    {
		Logger::debug("Client: " + Utils::toString(it->fd));
		result = 0;
        if (it->revents & POLLIN)
        {
			Logger::debug("POLLIN");
			if (fdIsServerSocket(it->fd) == true)
				createNewClient(it->fd);
			else if (fdIsCGI(it->fd) == true)
			{
				Logger::info("event on CGI");
				result = processCGI(it->fd);
				if (result == DONE)
				{
					std::vector<ClientHandler>::iterator clientIt = retrieveClientCGI(it->fd);
					for (int i = 0; i < poll_sets.size(); i++)
					{
						if (poll_sets[i].fd == clientIt->getFd())
						{
							poll_sets[i].events = POLLOUT;
							break;
						}
					}
				}
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
				else if (result == 3)
				{
					struct pollfd CGIPoll;

					CGIPoll.fd = retrieveClient(it->fd)->getCGI_Fd();
					CGIPoll.events = POLLIN;
					poll_sets.push_back(CGIPoll);
					Logger::info("CGI set up");
					Logger::debug("Client fd after setting up CGI: " + Utils::toString(it->fd));
					Logger::debug("CGI fd after setting up CGI: " + Utils::toString(retrieveClient(it->fd)->getCGI_Fd()));
				}
			}
        }
		else if (it->revents & POLLOUT)
		{
			Logger::error("POLLOUT");
			result = -2;
			Logger::error("result: " + Utils::toString(result));
			Logger::error("client fd: " + Utils::toString(it->fd));
			result = processClient(it->fd, WRITE);
			Logger::error("result after processClient: " + Utils::toString(result));
			Logger::error("fd: " + Utils::toString(it->fd));
			if (result == -1)
			{
				Logger::error("Something went wrong with send - don't know the meaning of it yet");
			}
			std::vector<ClientHandler>::iterator clientIt;
			clientIt = retrieveClient(it->fd);
			if (clientIt->getCGI_Fd() > 0)
			{
				for (int i = 0; i < poll_sets.size(); i++)
				{
					Logger::warn(Utils::toString(poll_sets[i].fd));
					if (poll_sets[i].fd == clientIt->getCGI_Fd()) {
						close(clientIt->getCGI_Fd());
						poll_sets.erase(poll_sets.begin() + i);
						break;
					}
				}
				close(it->fd);
				poll_sets.erase(it);
				clientsList.erase(clientIt);
				return;
			}
			it->events = POLLIN;
		}
		else if (it->revents & POLLERR)
		{
			Logger::debug("POLLERR");
			Logger::error("Fd " + Utils::toString(it->fd) + ": error");
			removeClient(it);
			return;
		}
		else if (it->revents & POLLHUP)
		{
			Logger::debug("POLLHUP");
			Logger::error("Fd " + Utils::toString(it->fd) + ": the other end has closed the connection.\n");
			removeClient(it);
			return;
		}
		else if (it->revents & POLLNVAL)
		{
			Logger::debug("POLLINVAL");
			Logger::error("Fd " + Utils::toString(it->fd) + ": invalid request, fd not open");
			poll_sets.erase(it);
			return;
		}
		else
			Logger::debug("Nothing");
		it++;
	}
}

int Webserver::processClient(int fd, int event)
{
	int status = -2;
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClient(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return 0;
	}
	if (event == READ)
	{
		Logger::debug("event on pollin");
		status = clientIt->manageRequest();
	}
	else
	{
		Logger::debug("event on pollout");
		Logger::error("client fd: " + Utils::toString(fd));
		Logger::error("retrieve response");
		Logger::error("status before: " + Utils::toString(status));
		status = clientIt->retrieveResponse();
	}
	Logger::error("retrieve response " + Utils::toString(status));
	return status;
}

int Webserver::processCGI(int fd)
{
	int status;
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClientCGI(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return 0;
	}
	status = clientIt->readStdout(fd);
	if (status == DONE)
		status = clientIt->createResponse();
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

bool Webserver::fdIsCGI(int fd)
{
	std::vector<ClientHandler>::iterator iterClient;
	std::vector<ClientHandler>::iterator endClient = this->clientsList.end();

	Logger::debug("Checking if it's CGI for fd " + Utils::toString(fd));
	for (iterClient = this->clientsList.begin(); iterClient != endClient; iterClient++)
	{
		if (iterClient->getCGI_Fd() == fd)
		{
			Logger::debug("FD " + Utils::toString(fd) + " is CGI");
			return true;
		}
	}
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

std::vector<ClientHandler>::iterator Webserver::retrieveClientCGI(int fd)
{
	std::vector<ClientHandler>::iterator iterClient;
	std::vector<ClientHandler>::iterator endClient = this->clientsList.end();

	for (iterClient = this->clientsList.begin(); iterClient != endClient; iterClient++)
	{
		if (iterClient->getCGI_Fd() == fd)
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
	Logger::info("Client " + Utils::toString(it->fd) + " disconnected");
}

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
				if (clientIt->getCGI_Fd() > 0)
				{
					for (int i = 0; i < poll_sets.size(); i++)
					{
						Logger::warn(Utils::toString(poll_sets[i].fd));
						if (poll_sets[i].fd == clientIt->getCGI_Fd())
						{
							kill(clientIt->getPid(), SIGTERM);
							close(clientIt->getCGI_Fd());
							poll_sets.erase(poll_sets.begin() + i);
							break;
						}
					}
				}
				removeClient(it);
				break;
			}
		}
	}
}
