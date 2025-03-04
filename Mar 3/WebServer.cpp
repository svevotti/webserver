#include "WebServer.hpp"
#include <iostream>
#include <cstring>
#include <sstream>

Webserver::Webserver(InfoServer info) 
{
    ServerSockets serverFds(info);
    this->serverFds = serverFds.getServerSockets();
    this->totBytes = 0;
    this->full_buffer.clear();
    this->poll_sets.reserve(100);
    addServerSocketsToPoll(this->serverFds);
    std::cout << "[DEBUG] Webserver initialized with " << serverFds.getServerSockets().size() << " server FDs" << std::endl;
    for (size_t i = 0; i < serverFds.getServerSockets().size(); ++i)
        std::cout << "[DEBUG] serverFds[" << i << "] = " << serverFds.getServerSockets()[i] << std::endl;
}

Webserver::~Webserver() 
{
    for (size_t i = 0; i < poll_sets.size(); ++i) 
    {
        if (poll_sets[i].fd >= 0)
            close(poll_sets[i].fd);
    }
    for (std::map<int, CGITracker>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
        delete it->second.cgi;
    for (size_t i = 0; i < serverFds.size(); ++i) 
    {
        if (serverFds[i] >= 0)
            close(serverFds[i]);
    }
}

void Webserver::startServer(InfoServer info) 
{
    while (true) 
    {
        std::cout << "[LOG] Poll called" << std::endl;
        for (size_t i = 0; i < poll_sets.size(); ++i)
            std::cout << "[DEBUG] Poll set FD " << poll_sets[i].fd << " events: " << poll_sets[i].events << std::endl;
        int returnPoll = poll(&poll_sets[0], poll_sets.size(), 20 * 1000);
        std::cout << "[DEBUG] poll returned: " << returnPoll << ", poll_sets size: " << poll_sets.size() << std::endl;
        if (returnPoll == -1) 
        {
            std::cerr << "[ERROR] Poll failed" << std::endl;
            break;
        } 
        else if (returnPoll == 0) 
        {
            checkCGITimeouts();
            std::cout << "[LOG] No events" << std::endl;
        }
        else 
        {
            dispatchEvents(info);
            checkCGITimeouts();
        }
    }
}

void Webserver::addServerSocketsToPoll(const std::vector<int>& fds) 
{
    for (size_t i = 0; i < fds.size(); ++i) 
    {
        bool exists = false;
        for (size_t j = 0; j < poll_sets.size(); ++j) 
        {
            if (poll_sets[j].fd == fds[i]) 
            {
                exists = true;
                break;
            }
        }
        if (!exists) 
        {
            struct pollfd serverPoll;
            serverPoll.fd = fds[i];
            serverPoll.events = POLLIN;
            this->poll_sets.push_back(serverPoll);
            std::cout << "[DEBUG] Added server FD " << fds[i] << " to poll_sets" << std::endl;
        }
    }
}

int Webserver::createNewClient(int fd) 
{
    socklen_t clientSize = sizeof(struct sockaddr_storage);
    struct sockaddr_storage clientStruct;
    struct pollfd clientPoll;

    std::cout << "[DEBUG] Accepting client on server FD " << fd << std::endl;
    int clientFd = accept(fd, (struct sockaddr*)&clientStruct, &clientSize);
    if (clientFd == -1) 
    {
        std::cerr << "[ERROR] Accept failed" << std::endl;
        return -1;
    }
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    this->poll_sets.push_back(clientPoll);
    std::cout << "[LOG] Client created: " << clientFd << std::endl;
    return clientFd;
}

int Webserver::readData(int fd, std::string& str, int& bytes) 
{
    char buffer[BUFFER];
    std::memset(buffer, 0, sizeof(buffer));
    int res = recv(fd, buffer, BUFFER - 1, 0);
    if (res > 0) 
    {
        buffer[res] = '\0';
        str.append(buffer, res);
        bytes += res;
        std::cout << "[DEBUG] Read " << res << " bytes from fd " << fd << std::endl;
        return 1;
    }
    else if (res == 0)
        return 0;
    return -1;
}

void Webserver::handleReadEvents(int fd, InfoServer info) 
{
    std::cout << "[DEBUG] Handling read event for fd " << fd << std::endl;
    int bytesRecv = readData(fd, this->full_buffer, this->totBytes);
    if (bytesRecv == 0) 
    {
        std::cout << "[LOG] Socket " << fd << " closed connection" << std::endl;
        close(fd);
        this->it = this->poll_sets.erase(this->it);
        std::cout << "[DEBUG] After erase in handleReadEvents, poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
        return;
    }
    if (bytesRecv < 0) return;

    ClientRequest request;
    request.parseRequestHttp(this->full_buffer, this->totBytes);
    std::map<std::string, std::string> httpRequestLine = request.getRequestLine();

    if (httpRequestLine.find("Method") != httpRequestLine.end()) 
    {
        std::string method = httpRequestLine["Method"];
        std::string uri = httpRequestLine["Request-URI"];
        if (uri.find(".py") != std::string::npos)
            handleCGIRequest(request, fd, info);
        else 
        {
            ServerResponse serverResponse;
            std::string response;

            if (method == "GET")
                response = serverResponse.responseGetMethod(info, request);
            else if (method == "POST")
                response = serverResponse.responsePostMethod(info, request);
            else if (method == "DELETE")
                response = serverResponse.responseDeleteMethod(info, request);
            else
                response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";

            if (!response.empty())
                queueResponse(fd, response, method);
        }
        for (size_t i = 0; i < poll_sets.size(); ++i) 
        {
            if (poll_sets[i].fd == fd) 
            {
                poll_sets[i].events &= ~POLLIN;
                std::cout << "[DEBUG] Cleared POLLIN for client fd " << fd << ", new events: " << poll_sets[i].events << std::endl;
                break;
            }
        }
    }
    this->totBytes = 0;
    this->full_buffer.clear();
}

void Webserver::handleCGIRequest(const ClientRequest& request, int client_fd, InfoServer info) 
{
    CGI* cgi = 0;
    try 
    {
        std::cout << "[DEBUG] Starting CGI for client " << client_fd << std::endl;
        cgi = new CGI(request, "/tmp/uploads", info);
    } 
    catch (const CGIException& e) 
    {
        std::cerr << "[ERROR] CGI initialization failed: " << e.what() << std::endl;
        queueResponse(client_fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n", "");
        delete cgi;
        return;
    }

    CGITracker tracker;
    tracker.cgi = cgi;
    tracker.fd = client_fd;
    tracker.input_data = cgi->getProcessedBody();

    if (!tracker.input_data.empty()) 
    {
        struct pollfd in_fd;
        in_fd.fd = cgi->getInPipeWriteFd();
        in_fd.events = POLLOUT;
        poll_sets.push_back(in_fd);
        cgi_map[in_fd.fd] = tracker;
        std::cout << "[DEBUG] Added CGI input FD " << in_fd.fd << " to poll_sets" << std::endl;
    } 
    else
    {
        cgi->closePipe(cgi->getInPipeWriteFd());
        std::cout << "[DEBUG] No input data, closed CGI input FD" << std::endl;
    }

    struct pollfd out_fd;
    out_fd.fd = cgi->getOutPipeReadFd();
    out_fd.events = POLLIN;
    poll_sets.push_back(out_fd);
    cgi_map[out_fd.fd] = tracker;
    std::cout << "[DEBUG] Added CGI output FD " << out_fd.fd << " to poll_sets" << std::endl;

    std::cout << "[LOG] CGI request queued for client " << client_fd << " (PID " << cgi->getPid() << ")" << std::endl;
}

void Webserver::queueResponse(int fd, const std::string& response, const std::string& method) 
{
    ResponseTracker tracker;
    tracker.fd = fd;
    tracker.response = response;
    tracker.method = method;
    response_queue.push_back(tracker);
    for (size_t i = 0; i < poll_sets.size(); ++i) 
    {
        if (poll_sets[i].fd == fd) 
        {
            poll_sets[i].events |= POLLOUT;
            std::cout << "[DEBUG] Queued response for fd " << fd << ", new events: " << poll_sets[i].events << std::endl;
            break;
        }
    }
}

void Webserver::handleSendResponse(std::vector<struct pollfd>::iterator& it) 
{
    for (size_t i = 0; i < response_queue.size(); ++i) 
    {
        if (response_queue[i].fd == it->fd) 
        {
            send(it->fd, response_queue[i].response.c_str(), response_queue[i].response.size(), 0);
            std::cout << "[LOG] Sent " << response_queue[i].method << " response to " << it->fd << std::endl;
            response_queue.erase(response_queue.begin() + i);
            it->events &= ~POLLOUT;
            bool isServerFd = false;
            for (size_t j = 0; j < serverFds.size(); ++j) 
            {
                if (serverFds[j] == it->fd) 
                {
                    isServerFd = true;
                    break;
                }
            }
            if (!isServerFd) 
            {
                close(it->fd);
                it = poll_sets.erase(it);
                std::cout << "[DEBUG] After erase in handleSendResponse, poll_sets size: " << poll_sets.size() << std::endl;
                for (size_t j = 0; j < poll_sets.size(); ++j)
                    std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
            }
            else
                ++it;
            return;
        }
    }
    ++it;
}

void Webserver::checkCGITimeouts() 
{
    for (std::map<int, CGITracker>::iterator it = cgi_map.begin(); it != cgi_map.end(); ) 
    {
        CGI* cgi = it->second.cgi;
        if (cgi->isTimedOut()) 
        {
            std::cerr << "[ERROR] CGI timeout (PID " << cgi->getPid() << ") for client " << it->second.fd << std::endl;
            queueResponse(it->second.fd, "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n", "");
            delete cgi;

            for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ) 
            {
                if (pit->fd == cgi->getInPipeWriteFd() || pit->fd == cgi->getOutPipeReadFd()) 
                {
                    close(pit->fd);
                    pit = poll_sets.erase(pit);
                    std::cout << "[DEBUG] After erase in checkCGITimeouts, poll_sets size: " << poll_sets.size() << std::endl;
                    for (size_t j = 0; j < poll_sets.size(); ++j)
                        std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
                } 
                else
                    ++pit;
            }
            std::map<int, CGITracker>::iterator temp = it;
            ++it;
            cgi_map.erase(temp);
        } 
        else
            ++it;
    }
}

bool Webserver::isServerSocket(int fd) 
{
    for (size_t i = 0; i < serverFds.size(); ++i) 
    {
        if (serverFds[i] == fd) 
            return true;
    }
    return false;
}

void Webserver::handleServerPollin(std::vector<struct pollfd>::iterator& it) 
{
    std::cout << "[DEBUG] Server socket FD " << it->fd << " has POLLIN" << std::endl;
    createNewClient(it->fd);
    ++it;
}

bool Webserver::isCGIOutputFd(int fd, CGITracker& tracker) 
{
    if (cgi_map.find(fd) != cgi_map.end()) 
    {
        tracker = cgi_map[fd];
        return fd == tracker.cgi->getOutPipeReadFd();
    }
    return false;
}

void Webserver::readCGIOutput(std::vector<struct pollfd>::iterator& it, CGITracker& tracker) 
{
    std::cout << "[DEBUG] Reading from CGI output FD " << it->fd << std::endl;
    char buffer[BUFFER];
    std::memset(buffer, 0, sizeof(buffer));
    int res = read(it->fd, buffer, BUFFER - 1);
    if (res > 0) 
    {
        tracker.cgi->appendOutput(buffer, res);
        ++it;
    } 
    else if (res == 0) // EOF
    {
        tracker.cgi->setOutputDone(true);
        close(it->fd);
        it = poll_sets.erase(it);
        std::cout << "[DEBUG] After erase in readCGIOutput, poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
        if (tracker.cgi->isDone()) 
        {
            std::ostringstream oss;
            oss << tracker.cgi->getOutput().size();
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " +
                                   oss.str() +
                                   "\r\n\r\n" + tracker.cgi->getOutput();
            queueResponse(tracker.fd, response, "CGI");
            std::cout << "[LOG] CGI completed for client " << tracker.fd << std::endl;

            for (std::map<int, CGITracker>::iterator cit = cgi_map.begin(); cit != cgi_map.end(); ) 
            {
                if (cit->second.cgi == tracker.cgi)
                {
                    std::map<int, CGITracker>::iterator temp = cit;
                    ++cit;
                    cgi_map.erase(temp);
                }
                else
                    ++cit;
            }
            delete tracker.cgi;
        }
    } 
    else
        ++it;
}

void Webserver::handlePollinEvents(std::vector<struct pollfd>::iterator& it, InfoServer info) 
{
    if (isServerSocket(it->fd)) 
        handleServerPollin(it);
    else 
    {
        CGITracker tracker;
        if (isCGIOutputFd(it->fd, tracker)) 
            readCGIOutput(it, tracker);
        else
            handleReadEvents(it->fd, info);
    }
}

bool Webserver::isCGIInputFd(int fd, CGITracker& tracker) 
{
    if (cgi_map.find(fd) != cgi_map.end()) 
    {
        tracker = cgi_map[fd];
        return fd == tracker.cgi->getInPipeWriteFd();
    }
    return false;
}

void Webserver::writeCGIInput(std::vector<struct pollfd>::iterator& it, CGITracker& tracker) 
{
    std::cout << "[DEBUG] Writing to CGI input FD " << it->fd << std::endl;
    tracker.cgi->writeInput(tracker.input_data, it->fd);
    if (tracker.cgi->isInputDone())
    {
        it = poll_sets.erase(it);
        std::cout << "[DEBUG] After erase in writeCGIInput, poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
    }
    else
        ++it;
}

void Webserver::handlePolloutEvents(std::vector<struct pollfd>::iterator& it) 
{
    CGITracker tracker;
    if (isCGIInputFd(it->fd, tracker)) 
        writeCGIInput(it, tracker);
    else
        handleSendResponse(it);
}

void Webserver::handleCGIOutputHup(std::vector<struct pollfd>::iterator& it, CGITracker& tracker) 
{
    std::cout << "[DEBUG] Handling POLLHUP on CGI output FD " << it->fd << std::endl;
    char buffer[BUFFER];
    std::memset(buffer, 0, sizeof(buffer));
    int res = read(it->fd, buffer, BUFFER - 1);
    if (res > 0)
        tracker.cgi->appendOutput(buffer, res);
    tracker.cgi->setOutputDone(true);
    close(it->fd);
    it = poll_sets.erase(it);
    std::cout << "[DEBUG] After erase in handleCGIOutputHup, poll_sets size: " << poll_sets.size() << std::endl;
    for (size_t j = 0; j < poll_sets.size(); ++j)
        std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
    if (tracker.cgi->isDone()) 
    {
        std::ostringstream oss;
        oss << tracker.cgi->getOutput().size();
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " +
                               oss.str() +
                               "\r\n\r\n" + tracker.cgi->getOutput();
        queueResponse(tracker.fd, response, "CGI");
        std::cout << "[LOG] CGI completed for client " << tracker.fd << std::endl;

        for (std::map<int, CGITracker>::iterator cit = cgi_map.begin(); cit != cgi_map.end(); ) 
        {
            if (cit->second.cgi == tracker.cgi)
            {
                std::map<int, CGITracker>::iterator temp = cit;
                ++cit;
                cgi_map.erase(temp);
            }
            else
                ++cit;
        }
        delete tracker.cgi;
    }
}

void Webserver::handlePollhupEvents(std::vector<struct pollfd>::iterator& it) 
{
    std::cout << "[DEBUG] POLLHUP on FD " << it->fd << std::endl;
    CGITracker tracker;
    if (isCGIOutputFd(it->fd, tracker)) 
        handleCGIOutputHup(it, tracker);
    else 
    {
        close(it->fd);
        it = poll_sets.erase(it);
        std::cout << "[DEBUG] After erase in handlePollhupEvents, poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
    }
}

void Webserver::handlePollerrEvents(std::vector<struct pollfd>::iterator& it) 
{
    std::cout << "[ERROR] POLLERR on FD " << it->fd << std::endl;
    bool isServerFd = isServerSocket(it->fd);
    CGITracker tracker;
    bool isCGIOut = isCGIOutputFd(it->fd, tracker);
    bool isCGIIn = isCGIInputFd(it->fd, tracker);

    if (isServerFd) 
    {
        std::cerr << "[ERROR] Server socket FD " << it->fd << " encountered an error, closing server" << std::endl;
        close(it->fd);
        it = poll_sets.erase(it);
        // Optionally exit or reinitialize server socket here if critical
    }
    else if (isCGIOut || isCGIIn) 
    {
        std::cerr << "[ERROR] CGI FD " << it->fd << " (PID " << tracker.cgi->getPid() << ") errored, cleaning up" << std::endl;
        close(it->fd);
        it = poll_sets.erase(it);
        std::cout << "[DEBUG] After erase in handlePollerrEvents (CGI), poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
        
        for (std::map<int, CGITracker>::iterator cit = cgi_map.begin(); cit != cgi_map.end(); ) 
        {
            if (cit->second.cgi == tracker.cgi)
            {
                std::map<int, CGITracker>::iterator temp = cit;
                ++cit;
                cgi_map.erase(temp);
            }
            else
                ++cit;
        }
        delete tracker.cgi;
        queueResponse(tracker.fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n", "CGI Error");
    }
    else 
    {
        std::cerr << "[ERROR] Client FD " << it->fd << " errored, closing" << std::endl;
        close(it->fd);
        it = poll_sets.erase(it);
        std::cout << "[DEBUG] After erase in handlePollerrEvents (client), poll_sets size: " << poll_sets.size() << std::endl;
        for (size_t j = 0; j < poll_sets.size(); ++j)
            std::cout << "[DEBUG] Remaining FD " << poll_sets[j].fd << " events: " << poll_sets[j].events << std::endl;
    }
}

void Webserver::dispatchEvents(InfoServer info) 
{
    end = poll_sets.end();
    for (it = poll_sets.begin(); it != end;) 
    {
        std::cout << "[DEBUG] Checking FD " << it->fd << " with revents: " << it->revents << ", events: " << it->events << std::endl;
        if (it->revents & POLLIN && it->revents & it->events) 
            handlePollinEvents(it, info);
        else if (it->revents & POLLOUT && it->revents & it->events) 
            handlePolloutEvents(it);
        else if (it->revents & POLLHUP) 
            handlePollhupEvents(it);
        else if (it->revents & POLLERR) 
            handlePollerrEvents(it);
        else
            ++it;
    }
}