#include "WebServer.hpp"
#define BUFFER 1024
#define MAX 100

// Constructor and Destructor
Webserver::Webserver(InfoServer& info)
{
	this->_serverInfo = new InfoServer(info);
	ServerSockets serverFds(*this->_serverInfo);

	if (this->serverFds.size() < 0)
		Logger::error("Failed creating any server socket");
	this->serverFds = serverFds.getServerSockets();
	this->poll_sets.reserve(MAX);
	addServerSocketsToPoll(this->serverFds);
}

Webserver::~Webserver()
{
	delete this->_serverInfo;
	closeSockets();
}

// Setters and getters

// Main functions
void	Webserver::startServer()
{
	int returnPoll;

	while (1)
	{
		returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
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
		Logger::debug("call made by " + std::to_string(it->fd));
		Logger::debug("Length " + std::to_string(poll_sets.size()));
        if (it->revents & POLLIN)
        {
			if (fdIsServerSocket(it->fd) == true)
				createNewClient(it->fd);
			else
			{
				result = handleReadEvents(it->fd, it);
				if (result == 1)
				{
					Logger::info("Client " + std::to_string(it->fd) + " disconnected");
					close(it->fd);
					this->clientsQueue.erase(retrieveClient(it->fd));
					it = this->poll_sets.erase(it);
                    return ;
				}
			}
        }
		else if (it->revents & POLLOUT)
			handleWritingEvents(it->fd, it);
		if (result == 0)
			it++;
	}
}

void Webserver::handleWritingEvents(int fd, std::vector<struct pollfd>::iterator it)
{
	std::vector<struct client>::iterator iterClient;
	std::vector<struct client>::iterator endClient = this->clientsQueue.end();

	iterClient = retrieveClient(fd);
	if (iterClient == endClient)
	{
		Logger::error("Failed to find client " + std::to_string(fd));
		return;
	}
	//TODO:does it behave as recv?
	Logger::debug("bytes to send " + std::to_string(iterClient->response.size()));
	int bytes = send(fd, iterClient->response.c_str(), strlen(iterClient->response.c_str()), 0);
	if (bytes == -1)
		Logger::error("Failed send - Sveva check this out");
	Logger::info("these bytes were sent " + std::to_string(bytes));
	it->events = POLLIN;
	iterClient->response.clear();
	//clientsQueue.erase(iterClient);
}

int Webserver::handleReadEvents(int fd, std::vector<struct pollfd>::iterator it)
{
	std::string response;
	int contentLen = 0;
	int flag = 1;
	int bytesRecv;
	std::string full_buffer;
	int totBytes = 0;
	bytesRecv = readData(fd, full_buffer, totBytes);
	Logger::debug("Bytes " + std::to_string(totBytes));
	if (bytesRecv == 0)
		return 1;
	else if (bytesRecv == -1 && totBytes == 0)
	{
		Logger::debug("recv return -1");
		return 0;
	}
	else
	{
		std::vector<struct client>::iterator clientIt;
		clientIt = retrieveClient(fd);
		if (clientIt != this->clientsQueue.end())
			clientIt->request = ParsingRequest(full_buffer, totBytes);
		else
		{
			Logger::debug("Client " + std::to_string(clientIt->fd) + " not found");
			return 0;
		}
		if (isCgi(clientIt->request.getRequestLine()["Request-URI"]) == true)
			printf("send to CGI\n");
		else
		{
			struct client client;
			response = prepareResponse(clientIt->request);
			clientIt->response = response;
			client.fd = clientIt->fd;
			client.request = clientIt->request;
			client.response = clientIt->response;
			response.clear();
			this->clientsQueue.erase(clientIt);
			this->clientsQueue.push_back(client);
			it->events = POLLOUT;
			full_buffer.clear();
			totBytes = 0;
			Logger::info("Response created successfully and store in clientQueu");
		}
	}
	return 0;
}

ClientRequest Webserver::ParsingRequest(std::string str, int size)
{
	ClientRequest request;
	request.parseRequestHttp(str, size);
	return request;
}

std::string Webserver::prepareResponse(ClientRequest request)
{
	std::string response;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request.getRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		ServerResponse serverResponse(request, *this->_serverInfo);
		if (httpRequestLine["Method"] == "GET")
			response = serverResponse.responseGetMethod();
		else if (httpRequestLine["Method"] == "POST")
			response = serverResponse.responsePostMethod();
		else if (httpRequestLine["Method"] == "DELETE")
			response = serverResponse.responseDeleteMethod();
		else
			Logger::error("Method not found, Sveva, use correct status code line");
	}
	else
		Logger::error("Method not found, Sveva, use correct status code line");
	return response;
}

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
	struct client newClient;
	newClient.fd = clientFd;
	this->clientsQueue.push_back(newClient);
	Logger::info("New client " + std::to_string(clientFd) + " created and added to poll sets");
}

//TODO: add helper function to retrieve fd from vector
std::vector<struct client>::iterator Webserver::retrieveClient(int fd)
{
	std::vector<struct client>::iterator iterClient;
	std::vector<struct client>::iterator endClient = this->clientsQueue.end();

	for (iterClient = this->clientsQueue.begin(); iterClient != endClient; iterClient++)
	{
		if (iterClient->fd == fd)
			return iterClient;
	}
	return endClient;
}

int Webserver::readData(int fd, std::string &str, int &bytes)
{
	int res = 0;
	char buffer[BUFFER];

	while (1)
	{
		ft_memset(buffer, 0, sizeof(buffer));
		res = recv(fd, buffer, BUFFER, MSG_DONTWAIT);
		if (res <= 0)
			break;
		str.append(buffer, res);
		bytes += res;
	}
	return res;
}


int	Webserver::searchPage(std::string path)
{
	FILE *folder;

	folder = fopen(path.c_str(), "rb");
	if (folder == NULL)
		return false;
	fclose(folder);
	return true;
}

int Webserver::isCgi(std::string str)
{
	if (str.find(".py") != std::string::npos)
		return true;
	if (searchPage(this->_serverInfo->getServerDocumentRoot() + str) == true)
		return false;
	return true;
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