#include "Webserver.hpp"
#include <cstring>
#include <iostream> // Added for std::cerr

#define BUFFER 1024
#define MAX 100

//-------------Constructor and Destructor--------------------------------------------

Webserver::Webserver(InfoServer& info)
{
    this->_serverInfo = new InfoServer(info);
    ServerSockets serverFds(*this->_serverInfo);

    if (serverFds.getServerSockets().empty())
        Logger::error("Failed creating any server socket");

    this->serverFds = serverFds.getServerSockets();
    this->totBytes = 0;
    this->full_buffer.clear();
    this->poll_sets.reserve(MAX);
    addServerSocketsToPoll(this->serverFds);
}

Webserver::~Webserver()
{
    delete this->_serverInfo;

    for (std::vector<CGITracker>::iterator it = cgiQueue.begin(); it != cgiQueue.end(); ++it)
        delete it->cgi;
    closeSockets();
}

//------------------ Main functions----------------------------------------------

void Webserver::startServer()
{
    int returnPoll;
    Logger::info("Entering poll loop - debug version 2023-03-07");
    while (true)
    {
        std::ostringstream oss;
        oss << poll_sets.size();
        Logger::debug(std::string("Polling ") + oss.str() + " FDs");
        for (size_t i = 0; i < poll_sets.size(); ++i)
        {
            oss.str(""); oss << "FD " << poll_sets[i].fd << " events: " << poll_sets[i].events;
            Logger::debug(std::string("Poll set entry: ") + oss.str());
        }
        returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
        if (returnPoll == -1)
        {
            Logger::error(std::string("Failed poll: ") + std::string(strerror(errno)));
            break;
        }
        else if (returnPoll == 0)
            Logger::debug("Poll timeout: no events");
        else
        {
            oss.str(""); oss << returnPoll;
            Logger::debug(std::string("Poll detected ") + oss.str() + " events");
            dispatchEvents();
        }
    }
}

void Webserver::handlePollInEvent(int fd)
{
    std::ostringstream oss;
    oss << "Handling POLLIN for FD " << fd;
    Logger::debug(oss.str());
    if (fdIsServerSocket(fd) == true)
        createNewClient(fd);
    else
        handleReadEvents(fd);
}

void Webserver::handleCgiOutputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    std::string cgiResponse = "HTTP/1.1 ";
    std::ostringstream oss;
    if (this->it->revents & POLLERR)
    {
        cgiResponse += "500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        oss << fd;
        Logger::error(std::string("CGI error on FD ") + oss.str());
    }
    else // POLLHUP
    {
        std::string output = cgiIt->cgi->getOutput();
        Logger::debug(std::string("Raw CGI output: [") + output + "]");
        if (output.empty())
        {
            cgiResponse += "500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
            Logger::debug("CGI output empty, setting 500");
        }
        else
        {
            size_t bodyStart = output.find("\r\n\r\n");
            std::string status = "200 OK";
            if (bodyStart != std::string::npos)
            {
                std::string headers = output.substr(0, bodyStart);
                output = output.substr(bodyStart + 4);
                if (headers.find("HTTP/1.1 500") != std::string::npos)
                    status = "500 Internal Server Error";
                else if (headers.find("HTTP/1.1 400") != std::string::npos)
                    status = "400 Bad Request";
                Logger::debug(std::string("Stripped CGI headers: [") + headers + "]");
            }
            size_t realLength = output.size();
            while (realLength > 0 && !std::isprint(static_cast<unsigned char>(output[realLength - 1])))
                --realLength;
            output.resize(realLength);
            oss.str(""); oss << output.size();
            cgiResponse += status + "\r\nContent-Length: " + oss.str() +
                           "\r\nConnection: keep-alive\r\n\r\n" + output;
            Logger::debug(std::string("CGI completed, output size: ") + oss.str());
            Logger::debug(std::string("CGI output content: [") + output + "]");
            oss.str(""); oss << cgiResponse.size();
            Logger::debug(std::string("Full cgiResponse size: ") + oss.str());
        }
    }

    cgiIt->cgi->closePipe(cgiIt->fd);

    int clientFd = cgiIt->clientFd;
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Closing CGI pipe and storing client FD ") + oss.str());

    std::vector<struct ClientTracker>::iterator existing = retrieveClient(clientFd);
    if (existing != clientsQueue.end())
    {
        clientsQueue.erase(existing);
        oss.str(""); oss << clientFd;
        Logger::debug(std::string("Removed client FD ") + oss.str() + " from clientsQueue");
    }

    oss.str(""); oss << this->it->fd;
    Logger::debug(std::string("this->it->fd before poll_sets erasure: ") + oss.str());

    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == clientFd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << clientFd;
            Logger::debug(std::string("Removed FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
        {
            ++pit;
        }
    }

    oss.str(""); oss << this->it->fd;
    Logger::debug(std::string("this->it->fd after FD 4 erasure: ") + oss.str());

    CGI* cgiPtr = cgiIt->cgi;
    cgiQueue.erase(cgiIt);
    oss.str(""); oss << fd;
    Logger::debug(std::string("Erased CGI tracker for FD ") + oss.str());
    delete cgiPtr;
    Logger::debug("Deleted CGI object");

    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == fd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << fd;
            Logger::debug(std::string("Erased FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
        {
            ++pit;
        }
    }
    bool fd7Found = false;
    for (size_t i = 0; i < poll_sets.size(); ++i)
    {
        if (poll_sets[i].fd == fd)
        {
            fd7Found = true;
            break;
        }
    }
    oss.str(""); oss << fd;
    Logger::debug(std::string("FD ") + oss.str() + " still in poll_sets after erase: " + (fd7Found ? "yes" : "no"));

    struct ClientTracker client = { clientFd, ClientRequest(), cgiResponse };
    clientsQueue.push_back(client);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added client FD ") + oss.str() + " to clientsQueue");

    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLOUT;
    poll_sets.push_back(clientPoll);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added FD ") + oss.str() + " with POLLOUT to poll_sets");
    oss.str(""); oss << "FD " << poll_sets.back().fd << " events: " << poll_sets.back().events;
    Logger::debug(std::string("Verified poll_sets last entry: ") + oss.str());

    oss.str(""); oss << clientFd;
    Logger::info(std::string("CGI response queued for client FD ") + oss.str());
    oss.str(""); oss << poll_sets.size();
    Logger::debug(std::string("Poll sets size after queuing: ") + oss.str());
    for (size_t i = 0; i < poll_sets.size(); ++i)
    {
        oss.str(""); oss << "FD " << poll_sets[i].fd << " events: " << poll_sets[i].events;
        Logger::debug(std::string("Poll set entry after queuing: ") + oss.str());
    }
}

void Webserver::handlePollOutEvent(int fd)
{
    std::ostringstream oss;
    oss << "Handling POLLOUT for FD " << fd;
    Logger::debug(oss.str());

    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end())
    {
        if (cgiIt->fd == cgiIt->cgi->getInPipeWriteFd())
        {
            writeCGIInput(fd, cgiIt);
            return;
        }
        oss.str(""); oss << fd;
        Logger::warn(std::string("Unexpected POLLOUT on CGI output FD ") + oss.str());
        return;
    }

    std::vector<struct ClientTracker>::iterator clientIt = retrieveClient(fd);
    if (clientIt != clientsQueue.end())
    {
        sendClientResponse(fd, clientIt);
    }
    else
    {
        oss.str(""); oss << fd;
        Logger::debug(std::string("No CGI or client match for FD ") + oss.str() + ", skipping");
    }
}

void Webserver::handleClientError(int fd)
{
    std::ostringstream oss;
    oss << fd;
    Logger::info(std::string("Client ") + oss.str() + " hung up or errored");
    close(fd);
    std::vector<ClientTracker>::iterator clientIt = retrieveClient(fd);
    if (clientIt != clientsQueue.end())
    {
        this->clientsQueue.erase(clientIt);
    }
    bool removed = false;
    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
    {
        if (pit->fd == fd)
        {
            this->it = poll_sets.erase(pit);
            removed = true;
            break;
        }
    }
    if (!removed)
    {
        Logger::warn(std::string("FD ") + oss.str() + " not found in poll_sets");
    }
}

void Webserver::handlePollErrorOrHangup(int fd)
{
    std::ostringstream oss;
    oss << fd;
    Logger::debug(std::string("Handling POLLERR or POLLHUP for FD ") + oss.str());
    oss.str(""); oss << this->it->fd;
    Logger::debug(std::string("this->it->fd in handlePollErrorOrHangup: ") + oss.str());

    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end())
    {
        if (cgiIt->fd == cgiIt->cgi->getOutPipeReadFd())
            handleCgiOutputPipeError(fd, cgiIt);
        else if (cgiIt->fd == cgiIt->cgi->getInPipeWriteFd())
        {
            bool outputHandled = false;
            for (size_t i = 0; i < clientsQueue.size(); ++i)
            {
                if (clientsQueue[i].fd == cgiIt->clientFd)
                {
                    outputHandled = true;
                    break;
                }
            }
            if (!outputHandled)
                handleCgiInputPipeError(fd, cgiIt);
            else
            {
                Logger::debug(std::string("Skipping input pipe error for FD ") + oss.str() + " as output already handled");
                cgiIt->cgi->closePipe(fd);
                poll_sets.erase(this->it);
            }
        }
        else
        {
            Logger::warn(std::string("Unexpected CGI FD ") + oss.str() + " in poll error");
        }
    }
    else
    {
        handleClientError(fd);
    }
}

void Webserver::handleCgiInputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    std::ostringstream oss;
    oss << fd;
    Logger::error(std::string("CGI input pipe error on FD ") + oss.str());

    cgiIt->cgi->closePipe(cgiIt->fd);

    int clientFd = cgiIt->clientFd;
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Storing client FD ") + oss.str() + " for error response");

    CGI* cgiPtr = cgiIt->cgi;
    cgiQueue.erase(cgiIt);
    oss.str(""); oss << fd;
    Logger::debug(std::string("Erased CGI tracker for FD ") + oss.str());
    delete cgiPtr;
    Logger::debug("Deleted CGI object");

    std::string cgiResponse = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
    struct ClientTracker client = { clientFd, ClientRequest(), cgiResponse };
    clientsQueue.push_back(client);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added client FD ") + oss.str() + " to clientsQueue with 500 response");

    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLOUT;
    poll_sets.push_back(clientPoll);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added FD ") + oss.str() + " with POLLOUT to poll_sets");

    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == fd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << fd;
            Logger::debug(std::string("Erased FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
        {
            ++pit;
        }
    }

    oss.str(""); oss << clientFd;
    Logger::info(std::string("CGI error response queued for client FD ") + oss.str());
}

void Webserver::dispatchEvents()
{
    end = this->poll_sets.end();
    for (this->it = poll_sets.begin(); this->it != end; /* incrementing inside loop */)
    {
        std::vector<struct pollfd>::iterator current = this->it;
        std::ostringstream oss;
        oss << "FD " << this->it->fd << " revents: " << this->it->revents;
        Logger::debug(std::string("Checking ") + oss.str());

        if (this->it->revents & POLLIN)
        {
            std::vector<CGITracker>::iterator cgiIt = retrieveCGI(this->it->fd);
            if (cgiIt != cgiQueue.end())
            {
                if (this->it->fd == cgiIt->cgi->getOutPipeReadFd())
                {
                    handleCgiOutput(this->it->fd, cgiIt);
                    ++this->it;
                }
                else if (this->it->fd == cgiIt->cgi->getStderrFd())
                {
                    char buffer[256];
                    ssize_t bytes = read(this->it->fd, buffer, sizeof(buffer) - 1);
                    if (bytes > 0)
                    {
                        buffer[bytes] = '\0';
                        Logger::debug(std::string("CGI stderr: ") + std::string(buffer));
                        ++this->it;
                    }
                    else
                    {
                        Logger::debug(std::string("Stderr pipe closed or errored for FD ") + oss.str());
                        close(this->it->fd);
                        this->it = poll_sets.erase(this->it);
                    }
                }
            }
            else
            {
                handlePollInEvent(this->it->fd);
                ++this->it;
            }
        }
        else if (this->it->revents & POLLOUT)
        {
            handlePollOutEvent(this->it->fd);
            ++this->it;
        }
        else if (this->it->revents & (POLLHUP | POLLERR | POLLNVAL))
        {
            Logger::debug(std::string("Detected POLLHUP, POLLERR, or POLLNVAL on FD ") + oss.str());
            handlePollErrorOrHangup(this->it->fd);
            if (current->fd == this->it->fd)
            {
                ++this->it;
            }
        }
        else if (this->it->revents != 0)
        {
            Logger::warn(std::string("Unexpected revents value ") + oss.str());
            handlePollErrorOrHangup(this->it->fd);
            if (current->fd == this->it->fd)
            {
                ++this->it;
            }
        }
        else
        {
            Logger::debug(std::string("No events for FD ") + oss.str());
            ++this->it;
        }
    }
}

void Webserver::handleClientDisconnect(int fd, int bytesRecv)
{
    if (bytesRecv == 0)
    {
        std::ostringstream oss;
        oss << fd;
        Logger::info(std::string("Client ") + oss.str() + " disconnected");
        close(fd);
        this->it = this->poll_sets.erase(this->it);
        if (retrieveClient(fd) != this->clientsQueue.end())
            this->clientsQueue.erase(retrieveClient(fd));
    }
}

void Webserver::processRequest(int fd, int contentLength)
{
    std::ostringstream oss;
    oss << this->totBytes;
    Logger::debug(std::string("recv this bytes: ") + oss.str());

    size_t clPos = this->full_buffer.find("Content-Length: ");
    if (clPos != std::string::npos)
    {
        contentLength = atoi(this->full_buffer.substr(clPos + 16).c_str());
        oss.str(""); oss << contentLength;
        Logger::debug(std::string("Parsed Content-Length: ") + oss.str());
    }
    else
    {
        contentLength = -1;
    }

    if ((contentLength == -1 && this->totBytes > 0) || (contentLength > 0 && this->totBytes >= contentLength))
    {
        std::vector<struct ClientTracker>::iterator clientIt = retrieveClient(fd);
        if (clientIt == this->clientsQueue.end())
        {
            oss.str(""); oss << fd;
            Logger::error(std::string("Client not found ") + oss.str() + " - please Sveva review");
            return;
        }
        clientIt->request = ParsingRequest(this->full_buffer, this->totBytes);
        oss.str(""); oss << "Request URI: " << clientIt->request.getRequestLine()["Request-URI"];
        Logger::debug(oss.str());
        if (isCGI(clientIt->request.getRequestLine()["Request-URI"]) == true)
        {
            setupCGI(fd, clientIt);
        }
        else
        {
            struct ClientTracker client;
            Logger::debug("fill in struct client");
            std::string response = prepareResponse(clientIt->request);
            clientIt->response = response;
            client.fd = clientIt->fd;
            client.request = clientIt->request;
            client.response = clientIt->response;
            response.clear();
            this->clientsQueue.erase(clientIt);
            this->clientsQueue.push_back(client);
            this->it->events = POLLOUT;
            this->full_buffer.clear();
            this->totBytes = 0;
            Logger::info("Response created successfully and stored in clientQueue");
        }
    }
    else
    {
        oss.str(""); oss << "Waiting for more data: " << this->totBytes << " of " << contentLength;
        Logger::debug(oss.str());
    }
}
void Webserver::setupCGI(int fd, std::vector<struct ClientTracker>::iterator& clientIt)
{
    std::ostringstream oss;
    oss << "Entering setupCGI for FD " << fd;
    Logger::debug(oss.str());

    try
    {
        std::string body = this->full_buffer; // Using the full raw request body
        std::cerr << "[DEBUG]: Full raw body size: " << body.size() << std::endl;

        std::string contentType = clientIt->request.getHeaders()["Content-Type"];
        std::string contentLengthStr = clientIt->request.getHeaders()["Content-Length"];
        int contentLength = atoi(contentLengthStr.c_str());
        oss.clear();
        oss.str("");
        oss << "Setting CGI env - Content-Type: " << contentType << ", Content-Length: " << contentLength;
        Logger::debug(oss.str());

        std::string bodyPreview = body.substr(0, 200);
        for (size_t i = 0; i < bodyPreview.size(); ++i)
        {
            if (!std::isprint(static_cast<unsigned char>(bodyPreview[i])))
                bodyPreview[i] = '.';
        }
        Logger::debug(std::string("Full body preview (first 200 bytes): [") + bodyPreview + "]");

        CGI* cgi = new CGI(clientIt->request, _serverInfo->getServerRootPath() + "/uploads", *_serverInfo, body);
        CGITracker tracker = { cgi, cgi->getInPipeWriteFd(), clientIt->fd, body };
        cgiQueue.push_back(tracker);

        struct pollfd cgiInPoll;
        cgiInPoll.fd = cgi->getInPipeWriteFd();
        cgiInPoll.events = POLLOUT;
        poll_sets.push_back(cgiInPoll);

        struct pollfd cgiOutPoll;
        cgiOutPoll.fd = cgi->getOutPipeReadFd();
        cgiOutPoll.events = POLLIN;
        poll_sets.push_back(cgiOutPoll);

        struct pollfd cgiErrPoll;
        cgiErrPoll.fd = cgi->getStderrFd();
        cgiErrPoll.events = POLLIN;
        poll_sets.push_back(cgiErrPoll);

        oss.clear();
        oss.str("");
        oss << "CGI setup for client FD " << fd << ", input FD " << cgiInPoll.fd << ", output FD " << cgiOutPoll.fd << ", stderr FD " << cgiErrPoll.fd;
        Logger::debug(oss.str());
        oss.clear();
        oss.str("");
        oss << poll_sets.size();
        Logger::debug(std::string("Poll sets size after CGI setup: ") + oss.str());

        this->full_buffer.clear();
        this->totBytes = 0;
    }
    catch (const std::exception& e)
    {
        oss.clear();
        oss.str("");
        oss << fd;
        Logger::error(std::string("CGI setup failed for FD ") + oss.str() + ": " + e.what());
        clientIt->response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == fd)
            {
                pit->events = POLLOUT;
                break;
            }
        }
        this->full_buffer.clear();
        this->totBytes = 0;
    }
}

void Webserver::handleCgiOutput(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    std::ostringstream oss;
    oss << "Read " << bytes << " from CGI FD " << fd;
    Logger::debug(oss.str());

    if (bytes > 0)
    {
        cgiIt->cgi->appendOutput(buffer, bytes);
        oss.clear();
        oss.str("");
        oss << "Appended " << bytes << " from CGI FD " << fd << " to CGI output";
        Logger::debug(oss.str());
    }
    else if (bytes == 0)
    {
        std::string cgiOutput = cgiIt->cgi->getOutput();
        oss.clear();
        oss.str("");
        oss << "CGI raw output: " << cgiOutput;
        Logger::debug(oss.str());

        for (std::vector<ClientTracker>::iterator clientIt = clientsQueue.begin(); clientIt != clientsQueue.end(); ++clientIt)
        {
            if (clientIt->fd == cgiIt->clientFd)
            {
                clientIt->response = cgiOutput;
                break;
            }
        }
        oss.clear();
        oss.str("");
        oss << "CGI response queued for client FD " << cgiIt->clientFd;
        Logger::info(oss.str());

        close(fd);
        delete cgiIt->cgi;
        cgiQueue.erase(cgiIt);
        poll_sets.erase(it);

        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == cgiIt->clientFd)
            {
                pit->events = POLLOUT;
                break;
            }
        }
    }
    else
    {
        oss.clear();
        oss.str("");
        oss << "Failed to read CGI output from FD " << fd;
        Logger::error(oss.str());
        close(fd);
        delete cgiIt->cgi;
        cgiQueue.erase(cgiIt);
        poll_sets.erase(it);
    }
}

void Webserver::handleReadEvents(int fd)
{
    std::string response;
    int contentLength = -1;
    int bytesRecv = readData(fd, this->full_buffer, this->totBytes);
    std::ostringstream oss;
    oss << "Read " << bytesRecv << " bytes from FD " << fd;
    Logger::debug(oss.str());
    if (bytesRecv == 0)
    {
        handleClientDisconnect(fd, bytesRecv);
    }
    else
    {
        size_t clPos = this->full_buffer.find("Content-Length: ");
        if (clPos != std::string::npos)
        {
            contentLength = atoi(this->full_buffer.substr(clPos + 16).c_str());
            oss.clear();
            oss.str("");
            oss << contentLength;
            Logger::debug(std::string("Content-Length from header: ") + oss.str());
        }
        if ((contentLength > 0 && this->totBytes >= contentLength) || (contentLength == -1 && bytesRecv > 0))
        {
            processRequest(fd, contentLength);
        }
        else if (bytesRecv < 0 && this->totBytes > 0)
        {
            processRequest(fd, contentLength);
        }
        else
        {
            Logger::debug(std::string("No data or error on FD ") + oss.str() + ", waiting for next poll");
        }
    }

    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end() && cgiIt->fd == cgiIt->cgi->getOutPipeReadFd())
        handleCgiOutput(fd, cgiIt);
}

void Webserver::writeCGIInput(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    if (cgiIt != cgiQueue.end() && cgiIt->fd == cgiIt->cgi->getInPipeWriteFd())
    {
        std::ostringstream oss;
        oss << "Writing to CGI input FD " << fd;
        Logger::debug(oss.str());
        cgiIt->cgi->writeInput();
        if (cgiIt->cgi->isInputDone())
        {
            int outFd = cgiIt->cgi->getOutPipeReadFd();
            this->it->fd = outFd;
            this->it->events = POLLIN;
            cgiIt->fd = outFd;
            oss.clear();
            oss.str("");
            oss << outFd;
            Logger::debug(std::string("Switched to CGI output FD ") + oss.str() + " with POLLIN");

            // Logging the full body sent to CGI
            std::string bodyPreview = cgiIt->input_data;
            oss.clear();
            oss.str("");
            oss << cgiIt->input_data.size();
            for (size_t i = 0; i < bodyPreview.size(); ++i)
            {
                if (!std::isprint(static_cast<unsigned char>(bodyPreview[i])))
                    bodyPreview[i] = '.';
            }
            Logger::debug(std::string("Full body sent to CGI (size ") + oss.str() + "): [" + bodyPreview + "]");

            oss.clear();
            oss.str("");
            oss << poll_sets.size();
            Logger::debug(std::string("Poll sets size after switch: ") + oss.str());
            for (size_t i = 0; i < poll_sets.size(); ++i)
            {
                oss.clear();
                oss.str("");
                oss << "FD " << poll_sets[i].fd << " events: " << poll_sets[i].events;
                Logger::debug(std::string("Poll set entry: ") + oss.str());
            }
        }
        else
        {
            oss.clear();
            oss.str("");
            oss << "Input not done for FD " << fd;
            Logger::debug(oss.str());
        }
    }
    else
    {
        std::ostringstream oss;
        oss << fd;
        Logger::debug(std::string("No CGI match for FD ") + oss.str());
    }
}

void Webserver::sendClientResponse(int fd, std::vector<struct ClientTracker>::iterator& iterClient)
{
    std::ostringstream oss;
    oss << fd;
    Logger::debug(std::string("Sending response for FD ") + oss.str());
    oss.clear();
    oss.str("");
    oss << iterClient->response.size();
    Logger::debug(std::string("bytes to send ") + oss.str());

    if (iterClient->response.empty())
    {
        Logger::debug(std::string("No response to send for FD ") + oss.str());
        close(fd);
        this->it = poll_sets.erase(this->it);
        clientsQueue.erase(iterClient);
        return;
    }

    int bytes = send(fd, iterClient->response.c_str(), iterClient->response.size(), MSG_DONTWAIT);
    if (bytes == -1)
    {
        Logger::error(std::string("Failed send for FD ") + oss.str() + ": " + std::string(strerror(errno)));
        close(fd);
        this->it = poll_sets.erase(this->it);
        clientsQueue.erase(iterClient);
    }
    else
    {
        oss.clear();
        oss.str("");
        oss << bytes;
        Logger::info(std::string("these bytes were sent ") + oss.str());
        oss.clear();
        oss.str("");
        oss << fd;
        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == fd)
            {
                pit->events = POLLIN;
                Logger::debug(std::string("Switched FD ") + oss.str() + " to POLLIN after sending");
                break;
            }
        }
        clientsQueue.erase(iterClient);
    }
}

void Webserver::handleWritingEvents(int fd)
{
    std::vector<struct ClientTracker>::iterator iterClient;
    std::vector<struct ClientTracker>::iterator endClient = this->clientsQueue.end();

    iterClient = retrieveClient(fd);
    if (iterClient == endClient)
    {
        std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
        writeCGIInput(fd, cgiIt);
        return;
    }
    sendClientResponse(fd, iterClient);
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

//---------------------------------Helpers-----------------------------

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
    int clientFd;

    clientSize = sizeof(clientStruct);
    clientFd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize);
    if (clientFd == -1)
    {
        Logger::error(std::string("Failed to create new client (accept): ") + std::string(strerror(errno)));
        return;
    }
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    this->poll_sets.push_back(clientPoll);
    struct ClientTracker newClient;
    newClient.fd = clientFd;
    this->clientsQueue.push_back(newClient);
    std::ostringstream oss;
    oss << clientFd;
    Logger::info(std::string("New client ") + oss.str() + " created and added to poll sets");
}

std::vector<struct ClientTracker>::iterator Webserver::retrieveClient(int fd)
{
    std::vector<struct ClientTracker>::iterator iterClient;
    std::vector<struct ClientTracker>::iterator endClient = this->clientsQueue.end();

    for (iterClient = this->clientsQueue.begin(); iterClient != endClient; iterClient++)
    {
        if (iterClient->fd == fd)
            return iterClient;
    }
    return endClient;
}

std::vector<struct CGITracker>::iterator Webserver::retrieveCGI(int fd)
{
    std::vector<struct CGITracker>::iterator iterCGI;
    std::vector<struct CGITracker>::iterator endCGI = this->cgiQueue.end();

    for (iterCGI = this->cgiQueue.begin(); iterCGI != endCGI; iterCGI++)
    {
        if (iterCGI->fd == fd)
            return iterCGI;
    }
    return endCGI;
}

int Webserver::readData(int fd, std::string &str, int &bytes)
{
    int res = 0;
    char buffer[BUFFER];
    std::ostringstream oss;
    int contentLength = -1;
    size_t clPos = str.find("Content-Length: ");
    if (clPos != std::string::npos)
    {
        contentLength = atoi(str.substr(clPos + 16).c_str());
        oss.str(""); oss << contentLength;
        Logger::debug(std::string("Content-Length from header: ") + oss.str());
    }

    do
    {
        memset(buffer, 0, sizeof(buffer));
        res = recv(fd, buffer, BUFFER - 1, MSG_DONTWAIT);
        oss.clear();
        oss.str("");
        oss << "recv returned " << res << " for FD " << fd;
        Logger::debug(oss.str());
        if (res > 0)
        {
            str.append(buffer, res);
            bytes += res;
            oss.clear();
            oss.str("");
            oss << bytes;
            Logger::debug(std::string("Total bytes read: ") + oss.str());
        }
        else if (res == 0)
        {
            oss.clear();
            oss.str("");
            oss << fd;
            Logger::info(std::string("Client FD ") + oss.str() + " closed connection");
            break;
        }
        else
        {
            oss.clear();
            oss.str("");
            oss << fd;
            Logger::debug(std::string("recv done or no data yet for FD ") + oss.str());
            break;
        }
    } while (res > 0 && (contentLength == -1 || bytes < contentLength));

    oss.clear();
    oss.str("");
    oss << "Final buffer size: " << str.size();
    Logger::debug(oss.str());
    return res;
}

int Webserver::searchPage(std::string path)
{
    FILE *folder;

    folder = fopen(path.c_str(), "rb");
    if (folder == NULL)
        return false;
    fclose(folder);
    return true;
}

int Webserver::isCGI(std::string str)
{
    if (str == "/upload" || str.find(".py") != std::string::npos)
        return true;
    if (searchPage(this->_serverInfo->getServerDocumentRoot() + str) == true) 
    {
        return false;
    }
    return false;
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