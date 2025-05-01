#include "WebServer.hpp"
#include <ctime>
#include <signal.h>
#define BUFFER 1024
#define MAX 1024

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

// Main functions
// int checkChildProcesses() {
// 	std::cout << "check child process\n";
//     for (std::map<pid_t, t_pid>::iterator it = childProcesses.begin(); it != childProcesses.end();) {
// 		Logger::debug("fd cgi: " + Utils::toString(it->second.cgi_fd));
// 		Logger::debug("fd client: " + Utils::toString(it->second.clent_fd));
// 		Logger::debug("pid: " + Utils::toString(it->first));
//         int status;
//         pid_t result = waitpid(it->first, &status, WNOHANG); // Non-blocking wait

//         if (result == 0) {
//             // Child is still running
//             ++it; // Move to the next child
//         } else if (result == -1) {
//             // Error occurred
//             perror("waitpid failed");
//             it = childProcesses.erase(it); // Remove from the map
//         } else {
//             // Child has exited
//             std::string response_process = "HTTP/1.1 200 OK\r\n";
//             response_process += "Content-Type: text/html\r\n";
//             response_process += "Connection: keep-alive\r\n";
//             response_process += "\r\n"; // End of headers
//             response_process += "<!DOCTYPE html>\n";
//             response_process += "<html>\n";
//             response_process += "<head><title>Bye Child</title></head>\n";
//             response_process += "<body>\n";
//             response_process += "<h1>Bye Child</h1>\n";
//             response_process += "<form action=\"/submit\" method=\"post\">\n";
//             response_process += "  <input type=\"text\" name=\"message\" placeholder=\"Type your message here\">\n";
//             response_process += "  <input type=\"submit\" value=\"Send\">\n";
//             response_process += "</form>\n";
//             response_process += "</body>\n";
//             response_process += "</html>\n";
//             Logger::error("Bye child");
// 			// Logger::debug("fd: " + Utils::toString(it->second));
// 			// int fd = it->second;
// 			// Logger::debug("pid: " + Utils::toString(it->first));
//             send(it->second.clent_fd, response_process.c_str(), response_process.size(), 0); // Send response
//             it = childProcesses.erase(it); // Remove from the map
// 			return 0;
//         }
//     }
// 	return -1;
// }

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
	int result;

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
				Logger::info("Start CGI logic here");
				std::string response_process = "HTTP/1.1 200 OK\r\n";
				response_process += "Content-Type: text/html\r\n";
				response_process += "Connection: keep-alive\r\n";
				char buffer[5000];
				std::string html_page;
				int res;
				while (1)
				{
					Utils::ft_memset(buffer, 0, sizeof(buffer));
					res = read(it->fd, buffer, BUFFER - 1);
					if (res <= 0)
						break;
					html_page.append(buffer, res);
					// bytes += res;
				}
				// ssize_t bytesRead = read(it->fd, buffer, sizeof(buffer) - 1);
				// std::string str(buffer);
				response_process += "Content-Lenght: " + Utils::toString(html_page.size());
				response_process += "\r\n"; // End of headers
				response_process += html_page;
				Logger::debug("Response_process: " + response_process);

				// Get the client fd associated with this CGI process
				std::vector<ClientHandler>::iterator clientIt = retrieveClientCGI(it->fd);
				int clientFd = clientIt->getFd(); // Ensure you are accessing the correct client fd
				Logger::debug("ready to send");
				send(clientFd, response_process.c_str(), response_process.size(), 0); // Send response
				Logger::debug("sent");
				std::vector<struct pollfd> copypoll = poll_sets;
				close(it->fd);
				close(clientFd);
				// clientIt->resetCGIFD();
				Logger::debug("fd cgi: " + Utils::toString(it->fd));
				Logger::debug("fd client: " + Utils::toString(clientFd));
				poll_sets.erase(it);
				std::cout << copypoll.size() << std::endl;
				std::cout << poll_sets.size() << std::endl;
				for (int i = 0; i < poll_sets.size(); i++)
				{
					if (poll_sets[i].fd == clientFd)
						poll_sets.erase(poll_sets.begin() + i);
				}
				return;
				// exit(0);
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
					return ;
				}
			}
        }
		else if (it->revents & POLLOUT)
		{
			Logger::debug("POLLOUT");
			if (processClient(it->fd, WRITE) == -1)
			{
				Logger::error("Something went wrong with send - don't know the meaning of it yet");
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
	int status;
	std::vector<ClientHandler>::iterator clientIt;

	clientIt = retrieveClient(fd);
	if (clientIt == this->clientsList.end())
	{
		Logger::error("Fd: " + Utils::toString(fd) + "not found in clientList");
		return 0;
	}
	if (event == READ)
		status = clientIt->manageRequest(poll_sets);
	else
		status = clientIt->retrieveResponse();
	if (status == 3) //Set CGITracker
	{
		CGITracker tracker(NULL, clientIt->getCGI_Fd(), clientIt->getFd());
		_cgiQueue.push_back(tracker);
	}
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
				// child_done = true;
				kill(this->pid, SIGKILL);
				std::string response_process = "HTTP/1.1 200 OK\r\n";
				response_process += "Content-Type: text/html\r\n";
				response_process += "Connection: keep alive\r\n";
				response_process += "\r\n"; // End of headers
				response_process += "<!DOCTYPE html>\n";
				response_process += "<html>\n";
				response_process += "<head><title>Bye Child</title></head>\n";
				response_process += "<body>\n";
				response_process += "<h1>Bye Child</h1>\n";
				response_process += "<form action=\"/submit\" method=\"post\">\n";
				response_process += "  <input type=\"text\" name=\"message\" placeholder=\"Type your message here\">\n";
				response_process += "  <input type=\"submit\" value=\"Send\">\n";
				response_process += "</form>\n";
				response_process += "</body>\n";
				response_process += "</html>\n";
				send(it->fd, response_process.c_str(), response_process.size(), 0);
				Logger::debug("fd: " + Utils::toString(this->pid));
				Logger::debug("pid: " + Utils::toString(it->fd));
				removeClient(it);
			}
		}
	}
}
