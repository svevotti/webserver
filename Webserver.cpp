#include "WebServer.hpp"
#define BUFFER 1024
#define MAX 100

// Constructor and Destructor

Webserver::Webserver(InfoServer& info)
{
	this->serverInfo = InfoServer(info);
	ServerSockets serverFds(this->serverInfo);

	this->serverFds = serverFds.getServerSockets();
	this->poll_sets.reserve(MAX);
	addServerSocketsToPoll(this->serverFds);
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

	if (this->serverFds.size() == 0)
		return -1;
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
	std::vector<struct pollfd>::iterator ite;
	int result;

	ite = this->poll_sets.end();
    for (it = poll_sets.begin(); it != ite;) 
    {
		result = 0;
		Logger::debug("Client: " + Utils::toString(it->fd));
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
				result = processClient(it->fd);
				if (result == DISCONNECTED)
				{
					removeClient(it->fd);
					this->poll_sets.erase(it);
                    return ;
				}
				else if (result == STATIC)
					it->events = POLLOUT;
				else if (result == CGI)
				{
					Logger::info("Set up CGI here");
				}
			}
        }
		else if (it->revents & POLLOUT)
		{

			if (processResponse(it->fd) == -1)
			{
				Logger::error("Something went wrong with send - don't know the meaning of it yet");
			}
			// handleWritingEvents(it->fd);
			it->events = POLLIN;
		}
		it++;
	}
}

int Webserver::fdIsCGI(int fd)
{
	return false;
}

int Webserver::processClient(int fd)
{
	int status;
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClient(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return 0;
	}
	status = clientIt->clientStatus();
	return status;
}

int Webserver::processResponse(int fd)
{
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClient(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return -1;
	}
	int status = clientIt->retrieveResponse();
	return status;
}

// void Webserver::handleWritingEvents(int fd)
// {
// 	std::vector<ClientHandler>::iterator iterClient;
// 	std::vector<ClientHandler>::iterator endClient = this->clientsList.end();

// 	iterClient = retrieveClient(fd);
// 	if (iterClient == endClient)
// 	{
// 		Logger::error("Failed to find client " + Utils::toString(fd));
// 		return;
// 	}
// 	//TODO:does it behave as recv?
// 	Logger::debug("bytes to send " + Utils::toString(iterClient->response.size()));
// 	int bytes = send(fd, iterClient->response.c_str(), iterClient->response.size(), 0);
// 	if (bytes == -1)
// 		Logger::error("Failed send - Sveva check this out");
// 	Logger::info("these bytes were sent " + Utils::toString(bytes));
// 	iterClient->response.clear();
// 	iterClient->raw_data.clear();
// 	iterClient->totbytes = 0;
// 	iterClient->request.cleanProperties();
// }

//utilis
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
	if (clientsNumber == 0)
		return;
	for (int i = 0; i < clientsNumber; i++)
	{
		serverPoll[i].fd = fds[i];
		serverPoll[i].events = POLLIN;
		this->poll_sets.push_back(serverPoll[i]);
	}
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
	ClientHandler newClient(clientFd, this->serverInfo);
	this->clientsList.push_back(newClient);
	Logger::info("New client " + Utils::toString(newClient.fd) + " created and added to poll sets");
}

std::vector<ClientHandler>::iterator Webserver::retrieveClient(int fd)
{
	std::vector<ClientHandler>::iterator iterClient;
	std::vector<ClientHandler>::iterator endClient = this->clientsList.end();

	for (iterClient = this->clientsList.begin(); iterClient != endClient; iterClient++)
	{
		if (iterClient->fd == fd)
			return iterClient;
	}
	return endClient;
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

void Webserver::removeClient(int fd)
{
	Logger::info("Client " + Utils::toString(fd) + " disconnected");
	close(fd);
	this->clientsList.erase(retrieveClient(fd));
}