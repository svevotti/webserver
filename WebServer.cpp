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
}

Webserver::~Webserver()
{
	closeSockets();
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
	//NEW: +7 lines
	it = poll_sets.begin();
	//Simona says: Please keep this for CGI efficiency and server stability
	//Tracks consecutive POLLOUT writes, help avoid flooding the server with writes
	static int consecutiveWrites = 0;

	//for (it = poll_sets.begin(); it != poll_sets.end();)
	while(it != poll_sets.end())
	{
		Logger::debug("Processing FD: " + Utils::toString(it->fd));
		result = 0;
		//NEW: until after the "if (isCGI)" loop
		bool isCGI = fdIsCGI(it->fd);
		int	fd = it->fd;
		if (isCGI)
		{
			std::vector<CGITracker>::iterator cgiIt = cgiHandler.findCGI(fd);
			if (cgiIt != cgiHandler.getCGIQueue().end())
			{
				if (!cgiIt->isActive) // NEW: Skip if CGI is marked inactive
				{
					Logger::debug("Skipping inactive CGI FD " + Utils::toString(fd));
					it = poll_sets.erase(it);
					continue;
				}
				// Check if client exists
				bool clientExists = false;
				for (std::vector<ClientHandler>::iterator clientIt = clientsList.begin(); clientIt != clientsList.end(); ++clientIt)
				{
					if (clientIt->getFd() == cgiIt->clientFd)
					{
						clientExists = true;
						break;
					}
				}
				// If client doesn't exist, mark CGI as inactive
				if (!clientExists)
				{
					Logger::debug("No client found for CGI FD " + Utils::toString(fd) + ", marking inactive");
					cgiIt->isActive = false;
					it = poll_sets.erase(it);
					continue;
				}
			}
		}
		//For debugging - remove? +8 lines
		std::string reventsStr;
		if (it->revents & POLLIN) reventsStr += "POLLIN ";
		if (it->revents & POLLOUT) reventsStr += "POLLOUT ";
		if (it->revents & POLLHUP) reventsStr += "POLLHUP ";
		if (it->revents & POLLERR) reventsStr += "POLLERR ";
		if (it->revents & POLLNVAL) reventsStr += "POLLNVAL ";
		if (reventsStr.empty()) reventsStr = "NONE";
		Logger::debug("FD " + Utils::toString(fd) + " revents: " + reventsStr);

		//NEW: Check POLLHUP first for CGI FDs to prioritize output completion
		if (it->revents & POLLHUP)
		{
			//NEW: +1 line (Simona used this info instead of an error)
			//Logger::info("POLLHUP on " + std::string(isCGI ? "CGI" : "client") + " FD " + Utils::toString(fd) + (isCGI ? "" : ": client disconnected"));
			Logger::error("Fd " + Utils::toString(it->fd) + ": the other end has closed the connection.\n");
			//NEW: Added if loop for CGI
			if (isCGI)
				cgiHandler.handleCGIHangup(fd, poll_sets, clientsList);
			else
				removeClient(it);
			//NEW: +3 lines
			consecutiveWrites = 0;
			break;
			//return;
		}
		if (it->revents & POLLIN)
		{
			if (fdIsServerSocket(it->fd) == true)
			{
				createNewClient(it->fd);
				//NEW: +2 lines
				consecutiveWrites = 0;
				break;
			}
			if (isCGI)
			{
				//NEW: +6 lines
				Logger::info("Processing CGI read: " + Utils::toString(fd));
				cgiHandler.handleCGIOutput(fd, poll_sets, clientsList);
				consecutiveWrites = 0;
				//For debugging - remove?
				Logger::debug("Breaking after CGI read on FD " + Utils::toString(fd));
				break;
			}
			//NEW: remove else statement, add consecutiveWrites reset and logger record
			//else
			//{
			result = processClient(it->fd, READ);
			if (result == DISCONNECTED)
			{
				removeClient(it);
				consecutiveWrites = 0;
				//For debugging - remove?
				Logger::debug("Breaking after client disconnection of FD " + Utils::toString(fd));
				return ;
			}
			else if (result == 2)
				it->events = POLLOUT;
			//NEW: +2 lines
			consecutiveWrites = 0;
			++it;
			//}
		}
		else if (it->revents & POLLOUT)
		{
			//NEW: CGI loop
			if (isCGI) // trying
			{
				if (consecutiveWrites >= 3) // Allow 3 writes before skipping to check output FDs
				{
					Logger::debug("Skipping POLLOUT on FD " + Utils::toString(fd) + " after " + Utils::toString(consecutiveWrites) + " consecutive writes");
					consecutiveWrites = 0; // Reset to allow future writes
					++it;
					continue;
				}
				Logger::info("Processing CGI write: " + Utils::toString(fd));
				cgiHandler.handleCGIWrite(fd, poll_sets);
				consecutiveWrites++;
				//For debugging - remove?
				Logger::debug("Breaking after CGI write on FD " + Utils::toString(fd));
				break;
			}
			if (processClient(it->fd, WRITE) == -1)
			{
				//NEW: Leak-proofing this loop
				Logger::error("Send failed for client FD " + Utils::toString(fd));
				removeClient(it);
				consecutiveWrites = 0;
				break;
			}
			it->events = POLLIN;
			//NEW: +2 lines
			consecutiveWrites = 0;
			++it;
		}
		else if (it->revents & POLLERR)
		{
			//Added CGI check, reset consecutiveWrites and break instead of return
			Logger::error("POLLERR on " + std::string(isCGI ? "CGI" : "client") + " FD " + Utils::toString(it->fd));
			if (isCGI)
				cgiHandler.handleCGIError(fd, poll_sets, clientsList);
			else
				removeClient(it);
			consecutiveWrites = 0;
			break;
			//return;
		}
		else if (it->revents & POLLNVAL)
		{
			//NEW: Added CGI to this loop
			// Logger::error("Fd " + Utils::toString(it->fd) + ": invalid request, fd not open");
			// removeClient(it);
			// return;
			if (isCGI) // added by Simona - CGI integration
				Logger::error("POLLNVAL on FD " + Utils::toString(fd) + ": invalid FD");
			else
				Logger::debug("POLLNVAL on FD " + Utils::toString(fd) + ": invalid FD, likely client closed");
			if (isCGI) // ditto
				cgiHandler.handleCGIError(fd, poll_sets, clientsList);
			else if (retrieveClient(fd) != clientsList.end()) // Simona: Added check
				removeClient(it);
			else
				poll_sets.erase(it); // Simona: Added fallback for non-client FDs
			consecutiveWrites = 0;
			Logger::debug("Breaking after POLLNVAL on FD " + Utils::toString(fd));
			break;
		}
		//NEW: Else loop to add consecutive writes and increase the iterator to maintaint the logic of the while loop
		else
		{
			consecutiveWrites = 0;
			++it;
		}
		//it++;
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
