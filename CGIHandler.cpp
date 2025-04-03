#include "CGIHandler.hpp"

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() 
{
    std::vector<CGITracker>::iterator it;
    for (it = _cgiQueue.begin(); it != _cgiQueue.end(); ++it) 
        delete it->cgi;
    _cgiQueue.clear();
}

void CGIHandler::startCGI(ClientHandler& client, const std::string& raw_data, const InfoServer& serverConfig,
    std::vector<struct pollfd>& poll_sets)
{
    HttpRequest request = client.getRequest();
    std::string requestUri = request.getHttpRequestLine()["request-uri"];
    std::string method = request.getMethod();

    std::string uploadDir;
    std::map<std::string, Route> routes = serverConfig.getRoute();
    std::map<std::string, Route>::const_iterator routeIt = routes.find(client.findDirectory(requestUri));
    if (routeIt != routes.end() && routeIt->second.locSettings.find("upload_to") != routeIt->second.locSettings.end()) 
    {
        uploadDir = routeIt->second.locSettings.find("upload_to")->second;
    } 
    else if (method == "POST") 
    {
        Route cgiRoute = serverConfig.getCGI();
        uploadDir = serverConfig.getRoot() + (cgiRoute.path.empty() ? "/uploads" : cgiRoute.path);
    } 
    else 
        uploadDir = serverConfig.getRoot();
    
    Logger::debug("Upload dir determined: " + uploadDir);

    CGITracker tracker;
    try
    {
        tracker.cgi = new CGI(request, uploadDir, serverConfig, raw_data);
    }
    catch (const CGIException& e)
    {
        Logger::error("CGI initialization failed: " + std::string(e.what()));
        HttpResponse response(500, "CGI initialization failed");
        client.setResponse(response.composeRespone());
        for (std::vector<struct pollfd>::iterator pollIt = poll_sets.begin(); pollIt != poll_sets.end(); ++pollIt)
        {
            if (pollIt->fd == client.getFd())
            {
                pollIt->events = POLLOUT;
                break;
            }
        }
        return;
    }
    catch (const std::exception& e)
    {
        Logger::error("Unexpected error in CGI setup: " + std::string(e.what()));
        HttpResponse response(500, "Unexpected CGI error");
        client.setResponse(response.composeRespone());
        for (std::vector<struct pollfd>::iterator pollIt = poll_sets.begin(); pollIt != poll_sets.end(); ++pollIt)
        {
            if (pollIt->fd == client.getFd())
            {
                pollIt->events = POLLOUT;
                break;
            }
        }
        return;
    }

    tracker.pipeFd = tracker.cgi->getInPipeWriteFd();
    tracker.clientFd = client.getFd();
    tracker.response = "";

    struct pollfd cgiInPoll;
    cgiInPoll.fd = tracker.cgi->getInPipeWriteFd();
    cgiInPoll.events = POLLOUT;
    poll_sets.push_back(cgiInPoll);

    struct pollfd cgiOutPoll;
    cgiOutPoll.fd = tracker.cgi->getOutPipeReadFd();
    cgiOutPoll.events = POLLIN;
    poll_sets.push_back(cgiOutPoll);

    struct pollfd cgiErrPoll;
    cgiErrPoll.fd = tracker.cgi->getErrPipeReadFd();
    cgiErrPoll.events = POLLIN;
    poll_sets.push_back(cgiErrPoll);

    _cgiQueue.push_back(tracker);

    std::ostringstream oss;
    oss << "CGI started: clientFd=" << tracker.clientFd
        << ", inputFd=" << cgiInPoll.fd
        << ", outputFd=" << cgiOutPoll.fd
        << ", errorFd=" << cgiErrPoll.fd
        << ", uri=" << requestUri
        << ", uploadDir=" << uploadDir;
    Logger::info(oss.str());
}

void CGIHandler::handleCGIOutput(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
    std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
    if (cgiIt == _cgiQueue.end())
    {
        Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
        return;
    }

    if (cgiIt->cgi->isOutputDone())
    {
        Logger::debug("Output already done for CGI FD " + Utils::toString(fd) + ", skipping read");
        return;
    }

    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);

    int fdsToClose[3];
    initializeFDsToClose(cgiIt, fdsToClose);

    if (bytes > 0)
        handlePositiveBytes(fd, buffer, bytes, cgiIt, poll_sets, clients, pollIt, fdsToClose);
    else if (bytes == 0)
        handleEOF(fd, cgiIt, poll_sets, clients, pollIt, fdsToClose);
    else
    {
        Logger::warn("Read error on CGI output FD " + Utils::toString(fd) + " for clientFd=" + Utils::toString(cgiIt->clientFd));
        handleCGIError(fd, poll_sets, clients);
    }
}

std::vector<struct pollfd>::iterator CGIHandler::findPollIterator(int fd, std::vector<struct pollfd>& poll_sets)
{
    std::vector<struct pollfd>::iterator pollIt = poll_sets.begin();
    while (pollIt != poll_sets.end() && pollIt->fd != fd) 
        ++pollIt;
    return pollIt;
}

void CGIHandler::initializeFDsToClose(std::vector<CGITracker>::iterator cgiIt, int fdsToClose[3])
{
    fdsToClose[0] = cgiIt->cgi->getInPipeWriteFd();
    fdsToClose[1] = cgiIt->cgi->getOutPipeReadFd();
    fdsToClose[2] = cgiIt->cgi->getErrPipeReadFd();
}

void CGIHandler::handlePositiveBytes(int fd, char* buffer, ssize_t bytes,
    std::vector<CGITracker>::iterator cgiIt, std::vector<struct pollfd>& poll_sets,
    std::vector<ClientHandler>& clients, std::vector<struct pollfd>::iterator pollIt, int fdsToClose[3])
{
    cgiIt->cgi->appendOutput(buffer, bytes);
    Logger::debug("Read " + Utils::toString(bytes) + " bytes from CGI output FD " + Utils::toString(fd) + " for clientFd=" + Utils::toString(cgiIt->clientFd));
    bytes = read(fd, buffer, sizeof(buffer));
    if (bytes == 0)
    {
        handleEOF(fd, cgiIt, poll_sets, clients, pollIt, fdsToClose);
    }
    else if (bytes > 0)
    {
        cgiIt->cgi->appendOutput(buffer, bytes);
    }
    else
    {
        Logger::warn("Read failed on CGI output FD " + Utils::toString(fd));
        handleCGIError(fd, poll_sets, clients);
    }
}

void CGIHandler::handleEOF(int fd, std::vector<CGITracker>::iterator cgiIt,
    std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients,
    std::vector<struct pollfd>::iterator pollIt, int fdsToClose[3])
{
    cgiIt->cgi->setOutputDone();
    std::string cgiResponse = generateCGIResponse(cgiIt);
    setClientResponse(cgiIt, poll_sets, clients, cgiResponse);
    removeCGIFDsFromPollSets(poll_sets, fdsToClose);
    closeCGIFDs(fdsToClose, cgiIt);

    pollIt = poll_sets.begin();
    while (pollIt != poll_sets.end() && pollIt->fd != cgiIt->clientFd) ++pollIt;

    Logger::info("Removing CGI tracker for clientFd=" + Utils::toString(cgiIt->clientFd));
    removeCGI(cgiIt);
}

std::string CGIHandler::generateCGIResponse(std::vector<CGITracker>::iterator cgiIt)
{
    std::string output = cgiIt->cgi->getOutput();
    std::string status = "200 OK";
    std::string headers;
    std::string body;

    size_t bodyStart = output.find("\r\n\r\n");
    if (bodyStart != std::string::npos) 
    {
        headers = output.substr(0, bodyStart);
        body = output.substr(bodyStart + 4);
        if (headers.find("Status: 500") != std::string::npos)
            status = "500 Internal Server Error";
        else if (headers.find("Status: 400") != std::string::npos)
            status = "400 Bad Request";
    } 
    else 
        body = output.empty() ? "CGI returned no output" : output;

    HttpResponse response(status == "200 OK" ? 200 : (status == "400 Bad Request" ? 400 : 500), body);
    return response.composeRespone();
}

void CGIHandler::setClientResponse(std::vector<CGITracker>::iterator cgiIt,
    std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, const std::string& cgiResponse)
{
    bool clientFound = false;
    std::vector<ClientHandler>::iterator clientIt = clients.begin();
    for (; clientIt != clients.end(); ++clientIt)
    {
        if (clientIt->getFd() == cgiIt->clientFd)
        {
            clientIt->setResponse(cgiResponse);
            clientFound = true;
            Logger::info("Response set for client FD " + Utils::toString(cgiIt->clientFd) + ": 200 OK");
            break;
        }
    }

    if (clientFound)
    {
        std::vector<struct pollfd>::iterator pit = poll_sets.begin();
        for (; pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == cgiIt->clientFd)
            {
                pit->events = POLLOUT;
                Logger::info("Client FD " + Utils::toString(cgiIt->clientFd) + " set to POLLOUT");
                break;
            }
        }
    }
}

void CGIHandler::closeCGIFDs(int fdsToClose[3], std::vector<CGITracker>::iterator cgiIt)
{
    for (int i = 0; i < 3; ++i)
    {
        if (fdsToClose[i] != -1)
        {
            cgiIt->cgi->closePipe(fdsToClose[i]);
            fdsToClose[i] = -1;
        }
    }
}

void CGIHandler::removeCGIFDsFromPollSets(std::vector<struct pollfd>& poll_sets, int fdsToClose[3])
{
    std::vector<struct pollfd>::iterator tempIt = poll_sets.begin();
    while (tempIt != poll_sets.end())
    {
        bool found = false;
        for (int j = 0; j < 3; ++j)
        {
            if (fdsToClose[j] != -1 && tempIt->fd == fdsToClose[j])
            {
                tempIt = poll_sets.erase(tempIt);
                found = true;
                break;
            }
        }
        if (!found)
            ++tempIt;
    }
}

void CGIHandler::handleCGIError(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
    std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
    if (cgiIt == _cgiQueue.end())
    {
        Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
        return;
    }

    std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);

    if (fd == cgiIt->cgi->getErrPipeReadFd())
    {
        char buffer[256];
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0)
        {
            buffer[bytes] = '\0';
            Logger::debug("CGI stderr for clientFd=" + Utils::toString(cgiIt->clientFd) + ": " + std::string(buffer));
            return;
        }
    }

    bool clientFound = false;
    std::vector<ClientHandler>::iterator clientIt = clients.begin();
    for (; clientIt != clients.end(); ++clientIt)
    {
        if (clientIt->getFd() == cgiIt->clientFd)
        {
            if (clientIt->getResponse().empty())
            {
                HttpResponse response(500, "CGI execution failed");
                clientIt->setResponse(response.composeRespone());
            }
            clientFound = true;
            break;
        }
    }
    if (clientFound)
    {
        std::vector<struct pollfd>::iterator pit = poll_sets.begin();
        for (; pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == cgiIt->clientFd)
            {
                pit->events = POLLOUT;
                break;
            }
        }
    }

    cgiIt->cgi->closePipe(fd);
    if (pollIt != poll_sets.end())
        poll_sets.erase(pollIt);

    removeCGI(cgiIt);
    Logger::error("CGI error handled for clientFd=" + Utils::toString(cgiIt->clientFd) + " on FD " + Utils::toString(fd));
}

std::vector<CGITracker>::iterator CGIHandler::findCGI(int fd) 
{
    std::vector<CGITracker>::iterator it;
    for (it = _cgiQueue.begin(); it != _cgiQueue.end(); ++it) 
    {
        if (it->pipeFd == fd || it->cgi->getInPipeWriteFd() == fd ||
            it->cgi->getOutPipeReadFd() == fd || it->cgi->getErrPipeReadFd() == fd ||
            it->clientFd == fd) 
        {
            return it;
        }
    }
    return _cgiQueue.end();
}

void CGIHandler::removeCGI(std::vector<CGITracker>::iterator it) 
{
    delete it->cgi;
    _cgiQueue.erase(it);
}

void CGIHandler::handleCGIWrite(int fd, std::vector<struct pollfd>& poll_sets)
{
    std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
    if (cgiIt == _cgiQueue.end())
    {
        Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
        return;
    }

    std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);
    if (pollIt == poll_sets.end()) return;

    if (fd == cgiIt->cgi->getInPipeWriteFd())
    {
        cgiIt->cgi->writeInput();
        if (cgiIt->cgi->isInputDone())
        {
            cgiIt->cgi->closePipe(fd);
            poll_sets.erase(pollIt);
        }
    }
}

void CGIHandler::handleCGIHangup(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
    std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
    if (cgiIt == _cgiQueue.end())
    {
        Logger::info("No CGI tracker for FD " + Utils::toString(fd));
        return;
    }

    std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);
    if (pollIt == poll_sets.end()) return;

    if (cgiIt->cgi->isOutputDone())
    {
        Logger::info("CGI output complete on POLLHUP for FD " + Utils::toString(fd));
        poll_sets.erase(pollIt);
    }
    else
    {
        Logger::warn("CGI output incomplete on POLLHUP for FD " + Utils::toString(fd));
        handleCGIError(fd, poll_sets, clients);
    }
}