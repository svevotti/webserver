#include "WebServer.hpp"
#define BUFFER 1024
#define MAX 100

// Constructor and Destructor
//TODO: if there are no sockets. webserver should not start
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

// Setters and getters

// Main functions
int	Webserver::startServer()
{
	int returnPoll;

	if (this->serverFds.size() < 0)
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
			else
			{
				result = handleReadEvents(it->fd, it);
				if (result == 1)
				{
					Logger::info("Client " + Utils::toString(it->fd) + " disconnected");
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
		Logger::error("Failed to find client " + Utils::toString(fd));
		return;
	}
	//TODO:does it behave as recv?
	Logger::debug("bytes to send " + Utils::toString(iterClient->response.size()));
	int bytes = send(fd, iterClient->response.c_str(), strlen(iterClient->response.c_str()), 0);
	if (bytes == -1)
		Logger::error("Failed send - Sveva check this out");
	Logger::info("these bytes were sent " + Utils::toString(bytes));
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
	Logger::debug("Bytes " + Utils::toString(totBytes));
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
			Logger::debug("Client " + Utils::toString(clientIt->fd) + " not found");
			return 0;
		}
		if (isCgi(clientIt->request.getHttpHeaders()["Request-URI"]) == true)
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

HttpRequest Webserver::ParsingRequest(std::string str, int size)
{
	HttpRequest request;
	request.HttpParse(str, size);
	return request;
}

//prepare response: check for method: call specific functions, return status code somehow, call httpresponse
std::string Webserver::prepareResponse(HttpRequest request)
{
	std::string response;
	struct response data;

	std::map<std::string, std::string> httpRequestLine;
	httpRequestLine = request.getHttpRequestLine();
	if (httpRequestLine.find("Method") != httpRequestLine.end())
	{
		if (httpRequestLine["Method"] == "GET")
		{
			retrievePage(request, &data);
		}
		// else if (httpRequestLine["Method"] == "POST")
		// {
		// 	uploadFile(request, &data);
		// }
		// else if (httpRequestLine["Method"] == "DELETE")
		// {
		// 	deleteFile(request, &data);
		// }
		else
			Logger::error("Method not found, Sveva, use correct status code line");
		HttpResponse http(data.code, data.body);
		response = http.composeRespone();
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
	Logger::info("New client " + Utils::toString(clientFd) + " created and added to poll sets");
}

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
	if (searchPage(this->serverInfo.getServerDocumentRoot() + str) == true)
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

 /*to move these in client handler*/
 bool fileExists(std::string path)
{
	std::ifstream file;
	std::string line;
	std::string htmlFile;
	std::string temp;

	file.open(path.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
	if (!file)
	{
		Logger::error("Failed to open html file: " + std::string(strerror(errno)));
		return (false);
	}
	file.close();
	return true;
}

std::string extractContent(std::string path)
{
	std::ifstream file;
	std::string line;
	std::string htmlFile;
	std::string temp;

	file.open(path.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
	if (!file)
	{
		Logger::error("Failed to open html file: " + std::string(strerror(errno)));
		return (htmlFile);
	}
	while (std::getline(file, line))
	{
		if (line.size() == 0)
			continue;
		else
			htmlFile = htmlFile.append(line + "\r\n");
	}
	file.close();
	return (htmlFile);
}

void Webserver::retrievePage(HttpRequest request, struct response *data)
{
	std::string response;
	std::string htmlPage;
	int bodyHtmlLen;
	std::ostringstream intermediatestream;
	std::string strbodyHtmlLen;
	std::string httpHeaders;
	std::string statusCodeLine;
	std::string documentRootPath;
	std::string pathToTarget;
	struct stat pathStat;
	std::map<std::string, std::string> httpRequestLine;

	//TODO: need to check curl -O why is not downloading - not working cause i can't do it with getline
	httpRequestLine = request.getHttpRequestLine();
	documentRootPath = this->serverInfo.getServerDocumentRoot();
	pathToTarget = documentRootPath + httpRequestLine["Request-URI"];
	if (stat(pathToTarget.c_str(), &pathStat) != 0)
		Logger::error("Failed stat: " + std::string(strerror(errno)));
	if (S_ISDIR(pathStat.st_mode))
	{
		if (pathToTarget[pathToTarget.length()-1] == '/')
			pathToTarget += "index.html";
		else
		pathToTarget += "/index.html";
	}
	//check path exists
	if (fileExists(pathToTarget) == false)
	{
		data->code = 404;
		htmlPage = "./server_root/404.html";
	}
	else
	{
		data->code = 200;
		htmlPage = pathToTarget;
	}
	data->body = extractContent(htmlPage);
}