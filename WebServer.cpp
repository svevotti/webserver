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
	{
		status = clientIt->manageRequest();
		//NEW: +30 lines. For CGI
		if (status == 3 && clientIt->isCgi(clientIt->getRequest().getHttpRequestLine()["request-uri"]))
		{
			Logger::info("Starting CGI read for FD " + Utils::toString(fd) + " on Server " + clientIt->configInfo.getIP() + ":" + clientIt->configInfo.getPort());
			cgiHandler.startCGI(*clientIt, clientIt->configInfo, poll_sets);
			return 0;
		}
		if (status == DISCONNECTED)
		{
			char buffer[1];
			int result = recv(fd, buffer, 1, MSG_PEEK | MSG_DONTWAIT);
			if (result == 0) // Client disconnected
			{
				Logger::info("Client FD " + Utils::toString(fd) + " disconnected (detected in processClient)");
				for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end(); ++it)
				{
					if (it->fd == fd)
					{
						removeClient(it);
						break;
					}
				}
				return DISCONNECTED;
			}
			else
			{
				//For debugging - remove?
				Logger::debug("Disconnect check for FD " + Utils::toString(fd) + " returned non-zero");
				return 0; // Client still connected
			}
		}
	}
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

//NEW: function
int Webserver::fdIsCGI(int fd)
{
	std::vector<CGITracker>::iterator it;

	for (it = cgiHandler.getCGIQueue().begin(); it != cgiHandler.getCGIQueue().end(); ++it)
	{
		if (it->pipeFd == fd || it->cgi->getInPipeWriteFd() == fd ||
		it->cgi->getOutPipeReadFd() == fd || it->cgi->getErrPipeReadFd() == fd)
			return true;
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
	//NEW: +8 lines
	//In some cases it could return 0, e.g. if stdin is closed and the system assigns FD 0
	if (clientFd <= 0)
	{
		Logger::error("Invalid client FD " + Utils::toString(clientFd) + " from accept");
		if (clientFd == 0)
			close(clientFd);
		return;
	}
	clientPoll.fd = clientFd;
	clientPoll.events = POLLIN;
	this->poll_sets.push_back(clientPoll);
	std::vector<pollfd> newSet = poll_sets;
	InfoServer *server = matchFD(fd);
	//NEW: +7 lines
	//Add checks in case server is null (no match found)
	if (server == NULL)
	{
		Logger::error("No matching server found for FD " + Utils::toString(fd));
		close(clientFd);
		return;
	}
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
	//NEW: +38 lines
	int fd = it->fd;
	//close(it->fd);
	//this->clientsList.erase(retrieveClient(it->fd));
	//this->poll_sets.erase(it);
	std::vector<ClientHandler>::iterator clientIt = retrieveClient(it->fd);
	if (clientIt == clientsList.end())
	{
		Logger::warn("No client found for FD " + Utils::toString(it->fd) + " during removal");
		//close(it->fd);
		//poll_sets.erase(it); // CAUSES LEAK
		// NEW TRY
		close(fd);
		// Safely remove from poll_sets using index, not iterator
		for (std::vector<struct pollfd>::iterator pIt = poll_sets.begin(); pIt != poll_sets.end(); ++pIt)
		{
			if (pIt->fd == fd)
			{
				poll_sets.erase(pIt);
				break;
			}
		}
		return;
	}
	Logger::debug("Initiating CGI cleanup for client FD " + Utils::toString(it->fd));
	// Simona - Kill any CGI processes associated with this client before closing the FD
	cgiHandler.killCGIForClient(fd, poll_sets); // Simona - CGI integration
	close(fd);
	clientsList.erase(clientIt);
	//this->poll_sets.erase(it);
	//Simona leak debug Erase using a fresh iterator to avoid invalidation issues
	for (std::vector<struct pollfd>::iterator pIt = poll_sets.begin(); pIt != poll_sets.end(); ++pIt)
	{
		if (pIt->fd == fd)
		{
			poll_sets.erase(pIt);
			break;
		}
	}
}

// Setters and Getters

//SVEVA version
// void Webserver::checkTime(void)
// {
// 	std::time_t currentTime = std::time(NULL);
// 	std::vector<ClientHandler>::iterator clientIt;
// 	std::vector<struct pollfd>::iterator it;
// 	for (it = this->poll_sets.begin(); it < this->poll_sets.end(); it++)
// 	{
// 		clientIt = retrieveClient(it->fd);
// 		if (clientIt != this->clientsList.end())
// 		{
// 			if (currentTime - clientIt->getTime() > clientIt->getTimeOut())
// 			{
// 				Logger::error("Fd: " + Utils::toString(it->fd) + " timeout");
// 				removeClient(it);
// 			}
// 		}
// 	}
// }

//SIMONA version
// MODIFIED VERSION OF checkTime - Simona - CGI integration
// Microsecond precision timing - useful for CGI processing timeout
// CGI timeout checking (separate timeout handling for CGI clients)
// Inactive CGI trackers cleanup
// Skip webserver timeout check for CGI clients
// Cleanup stale CGI FDs to prevent memory leaks
// While instead of loop (to track better)
// Added logs and cgi_processing_timeout for CGI clients
void Webserver::checkTime(void)
{
	Logger::debug("Starting checkTime, poll_sets size: " + Utils::toString(poll_sets.size()) + ", clientsList size: " + Utils::toString(clientsList.size()));
	// Simona - Changed to use gettimeofday for microsecond precision
	// Explanation: This is important for CGI processing timeout (CGI scripts need more precise timing than standard clients)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double currentTime = tv.tv_sec + tv.tv_usec / 1000000.0;

	// Simona -- addition
	// Handle CGI-specific timeouts first
	// EXPLANATION: Must check CGI timeouts separately since they have different timeout rules
	cgiHandler.checkCGITimeout(poll_sets, clientsList, static_cast<time_t>(currentTime));

	// Simona - Clean up inactive CGI trackers that no longer have active clients
	// EXPLANATION: Prevent memory leaks from orphaned CGI trackers
	std::vector<CGITracker>& cgiQueue = cgiHandler.getCGIQueue();
	for (std::vector<CGITracker>::iterator cgiIt = cgiQueue.begin(); cgiIt != cgiQueue.end(); )
	{
		if (!cgiIt->isActive)
		{
			// Check if the client still exists
			bool clientExists = false;
			for (std::vector<ClientHandler>::iterator clientIt = clientsList.begin(); clientIt != clientsList.end(); ++clientIt)
			{
				if (clientIt->getFd() == cgiIt->clientFd)
				{
					clientExists = true;
					break;
				}
			}
			// Remove CGI tracker if client is gone
			if (!clientExists)
			{
				Logger::debug("Removing inactive CGI tracker for clientFd=" + Utils::toString(cgiIt->clientFd));
				cgiIt = cgiQueue.erase(cgiIt);
				continue;
			}
		}
		++cgiIt;
	}
	Logger::debug("Finished cleaning up inactive CGI trackers, remaining: " + Utils::toString(cgiQueue.size()));

	// Iterate through poll_sets to check for timeouts
	for (size_t i = 0; i < poll_sets.size(); ++i)
	{
		std::vector<struct pollfd>::iterator it = poll_sets.begin() + i;
		// Simona - debug for CGI integration -- was getting invalid fds issue this fixes it
		Logger::debug("Checking FD: " + Utils::toString(it->fd));

		// Simona - Skip invalid FDs
		if (it->fd <= 0)
		{
			Logger::debug("Removing invalid FD " + Utils::toString(it->fd) + " from poll_sets");
			poll_sets.erase(it);
			--i;
			continue;
		}

		// Check for CGI and server socket FDs
		bool isCGI = fdIsCGI(it->fd);
		bool isServerSocket = fdIsServerSocket(it->fd);
		if (isCGI)
		{
			Logger::debug("FD " + Utils::toString(it->fd) + " is a CGI FD, skipping timeout check");
			continue;
		}
		if (isServerSocket)
		{
			Logger::debug("FD " + Utils::toString(it->fd) + " is a server socket, skipping timeout check");
			continue;
		}

		std::vector<ClientHandler>::iterator clientIt = retrieveClient(it->fd);
		if (clientIt != clientsList.end())
		{
			// Simona - check for CGI active state
			// EXPLANATION: This is to determine if the client is currently processing a CGI request
			bool isCGIActive = false;
			std::vector<CGITracker>::iterator cgiIt;
			for (cgiIt = cgiQueue.begin(); cgiIt != cgiQueue.end(); ++cgiIt)
			{
				if (cgiIt->clientFd == it->fd && cgiIt->isActive)
				{
					isCGIActive = true;
					Logger::debug("CGI active for FD " + Utils::toString(it->fd));
					break;
				}
			}

			// Check for disconnected client, including CGI-active clients after response
			bool checkDisconnect = !isCGIActive || (isCGIActive && it->events == POLLOUT);
			if (checkDisconnect)
			{
				char buffer[1];
				int result = recv(it->fd, buffer, 1, MSG_PEEK | MSG_DONTWAIT);
				if (result == 0) // Client disconnected
				{
					Logger::info("Client FD " + Utils::toString(it->fd) + " disconnected (detected in checkTime)");
					cgiHandler.killCGIForClient(it->fd, poll_sets);
					close(it->fd);
					clientsList.erase(clientIt);
					poll_sets.erase(it);
					--i;
					continue;
				}
				else
					Logger::debug("Disconnect check for FD " + Utils::toString(it->fd) + " returned non-zero, events: " + (it->events == POLLOUT ? "POLLOUT" : "NOT POLLOUT"));
			}

			// Use appropriate timeout for CGI or non-CGI clients
			double timeoutToCheck = isCGIActive ? clientIt->getCGIProcessingTimeout() : clientIt->getTimeOut();
			double elapsed = currentTime - clientIt->getTime();
			if (elapsed >= timeoutToCheck)
			{
				Logger::error("FD: " + Utils::toString(it->fd) + (isCGIActive ? " CGI processing timeout" : " keepalive timeout"));
				if (isCGIActive)
				{
					Logger::debug("CGI active for FD " + Utils::toString(it->fd));
					cgiIt->cgi->killIfTimedOut();
					cgiIt->cgi->setOutputDone();
					cgiIt->cgi->closePipes();
					std::string response = cgiHandler.generateServerTimeoutResponse(cgiIt);
					clientIt->setResponse(response);
					if (write(it->fd, response.c_str(), response.length()) < 0)
					{
						Logger::error("Failed to write 503 response to FD " + Utils::toString(it->fd));
					}
					else
					{
						Logger::debug("Wrote " + Utils::toString(response.length()) + " bytes of 503 response to FD " + Utils::toString(it->fd));
					}
					cgiHandler.setClientToPollout(it->fd, poll_sets);
					int fdsToClose[3];
					cgiHandler.initializeFDsToClose(cgiIt, fdsToClose);
					cgiHandler.removeCGIFDsFromPollSets(poll_sets, fdsToClose);
					cgiHandler.closeCGIFDs(fdsToClose, cgiIt);
					cgiHandler.removeCGI(cgiIt);
					Logger::info("Disconnected client FD " + Utils::toString(it->fd) + " after CGI processing timeout");
					close(it->fd);
					clientsList.erase(clientIt);
					poll_sets.erase(it);
					--i;
				}
				else //Use simpler non-CGI timeout handling
				{
					if (!clientIt->getResponse().empty())
					{
						std::string responseStr = clientIt->getResponse();
						if (write(it->fd, responseStr.c_str(), responseStr.length()) < 0)
							Logger::error("Failed to write response to FD " + Utils::toString(it->fd));
						else
							Logger::debug("Wrote " + Utils::toString(responseStr.length()) + " bytes of response to FD " + Utils::toString(it->fd));
					}
					Logger::info("Client " + Utils::toString(it->fd) + " disconnected due to timeout");
					cgiHandler.killCGIForClient(it->fd, poll_sets); // Ensure CGI cleanup
					close(it->fd);
					clientsList.erase(clientIt);
					poll_sets.erase(it);
					--i;
				}
			}
			else
			{
				Logger::debug("FD: " + Utils::toString(it->fd) + " not timed out, elapsed: " + Utils::toString(elapsed) + ", timeout: " + Utils::toString(timeoutToCheck));
			}
		}
		else //Remove unexpected FDs
		{
			Logger::warn("Unexpected FD " + Utils::toString(it->fd) + " in poll_sets, removing");
			poll_sets.erase(it);
			--i;
		}
	}

	// Simona - Clean up inactive CGI FDs
	// EXPLANATION: This is a separate cleanup for CGI FDs that are no longer active
	//cgiHandler.cleanupInactiveCGIFDs(poll_sets);
	Logger::debug("Finished checkTime");
}
