#include "WebServer.hpp"
#include <ctime>
#define BUFFER 1024
#define MAX 1024

// Constructor and Destructor

Webserver::Webserver(Config& file)
{
	this->configInfo = file.getServList();
	std::string port = configInfo[0]->getPort();
	std::string ip = configInfo[0]->getIP();
	ServerSockets server(ip, port);

	this->serverFd = server.getServerSocket();
	this->poll_sets.reserve(MAX);
	if (this->serverFd > 0)
		addServerSocketsToPoll(this->serverFd);
	
	std::string errorPagePath = "/www/errors";
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
	double keepAliveTimeout = atof(this->configInfo[0]->getSetting()["keepalive_timeout"].c_str());
	for (it = this->poll_sets.begin(); it < this->poll_sets.end(); it++)
	{
		clientIt = retrieveClient(it->fd);
		if (clientIt != this->clientsList.end())
		{
			if (currentTime - clientIt->getTime() > keepAliveTimeout)
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

	if (this->serverFd < 0)
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
		checkTime();
	}
	return 0;
}

// Updated by Simona (CGI plus more logs and a couple of flags)
void Webserver::dispatchEvents()
{
    std::vector<struct pollfd>::iterator it = poll_sets.begin();

    Logger::debug("Starting dispatchEvents with " + Utils::toString(poll_sets.size()) + " FDs");
    
    while (it != poll_sets.end()) // Simona: Switched to while loop (easier for Simo)
    {
        Logger::debug("Processing FD: " + Utils::toString(it->fd)); // updated message

        //Simona: Moved FD type-checks outside event blocks (to keep function shorter)
        bool isServer = fdIsServerSocket(it->fd);
        bool isCGI = !isServer && fdIsCGI(it->fd);

        int fd = it->fd; // Simona: Stored fd to avoid repeated dereferencing, safer post-erase

        if (it->revents & POLLIN)
        {
            if (isServer) 
            { 
                createNewClient(fd); 
                break; 
            }
            if (isCGI) 
            { 
                Logger::info("Processing CGI read: " + Utils::toString(fd)); 
                cgiHandler.handleCGIOutput(fd, poll_sets, clientsList); 
                break; 
            }
            int result = processClient(fd, READ);
            if (result == DISCONNECTED) 
            { 
                removeClient(it); 
                break; 
            }
            if (result == 2) 
                it->events = POLLOUT; 
            ++it;
        }
        else if (it->revents & POLLOUT)
        {
            if (isCGI) 
            {
                Logger::info("Processing CGI write: " + Utils::toString(fd)); 
                cgiHandler.handleCGIWrite(fd, poll_sets); 
                break; 
            }
            int result = processClient(fd, WRITE);
            if (result == -1) 
            { 
                Logger::error("Send failed for client FD " + Utils::toString(fd)); 
                removeClient(it); 
                break; 
            }
            it->events = POLLIN;
            ++it;
        }
        else if (it->revents & POLLERR) //perhaps later from here on in a separate function
        {
            Logger::error("POLLERR on " + std::string(isCGI ? "CGI" : "client") + " FD " + Utils::toString(fd));
            if (isCGI) 
                cgiHandler.handleCGIError(fd, poll_sets, clientsList);
            else removeClient(it);
            break;
        }
        else if (it->revents & POLLHUP)
        {
            Logger::info("POLLHUP on " + std::string(isCGI ? "CGI" : "client") + " FD " + Utils::toString(fd) + (isCGI ? "" : ": client disconnected"));
            if (isCGI) 
                cgiHandler.handleCGIHangup(fd, poll_sets, clientsList);
            else 
                removeClient(it);
            break;
        }
        else if (it->revents & POLLNVAL)
        {
            Logger::error("POLLNVAL on FD " + Utils::toString(fd) + ": invalid FD");
            if (isCGI) 
                cgiHandler.handleCGIError(fd, poll_sets, clientsList);
            else if (retrieveClient(fd) != clientsList.end()) // Simona: Added check
                removeClient(it); 
            else 
                poll_sets.erase(it); // Simona: Added fallback for non-client FDs
            break;
        }
        else
            ++it;
    }
    Logger::debug("Finished dispatchEvents with " + Utils::toString(poll_sets.size()) + " FDs remaining");
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
		//***** START OF CHANGES BY SIMONA *****//
		if (status == 3 && clientIt->isCgi(clientIt->getRequest().getHttpRequestLine()["request-uri"]))
        {
			Logger::info("Processing CGI read for FD " + Utils::toString(fd));
            // Using friend access to pass configInfo to CGIHandler
            cgiHandler.startCGI(*clientIt, clientIt->getRawData(), clientIt->configInfo, poll_sets);
            return 0; // CGI is being processed
        }
		//**** END OF CHANGES BY SIMONA ****//
	}
	else
		status = clientIt->retrieveResponse();
	return status;
}

// Utils
int Webserver::fdIsServerSocket(int fd)
{
	if (fd == this->serverFd)
		return true;
	return false;
}

// Updated by Simona
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
	ClientHandler newClient(clientFd, *this->configInfo[0]);
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

// SVEVA's ORIGINAL VERSION (works well too)
// void Webserver::removeClient(std::vector<struct pollfd>::iterator it)
// {
//	Logger::info("Client " + Utils::toString(it->fd) + " disconnected");
//	close(it->fd);
//	this->clientsList.erase(retrieveClient(it->fd));
//	this->poll_sets.erase(it);
//}

// Updated version by Simona (more logs and safety checks)
// - added a check before erasing from clientsList to avoid crashes if the FD isn’t found
// - extra logs
void Webserver::removeClient(std::vector<struct pollfd>::iterator it)
{
    Logger::info("Removing client FD " + Utils::toString(it->fd));
    close(it->fd);
    std::vector<ClientHandler>::iterator clientIt = retrieveClient(it->fd);
    if (clientIt != clientsList.end())
        clientsList.erase(clientIt);
    poll_sets.erase(it);
}
