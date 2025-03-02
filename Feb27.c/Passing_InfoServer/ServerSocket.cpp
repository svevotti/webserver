#include "ServerSocket.hpp"
#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include "PrintingFunctions.hpp"
#include <vector>
#include <map>

#define SOCKET -1
#define GETADDRINFO -2
#define SETSOCKET -4
#define BIND -5
#define LISTEN -6
#define RECEIVE -7
#define SEND -8
#define ACCEPT -9
#define POLL -10
#define BUFFER 1024

int createServerSocket(const char* portNumber)
{
    int serverFd;
    struct addrinfo hints, *serverInfo;
    int error;
    int opt = 1;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    error = getaddrinfo(NULL, portNumber, &hints, &serverInfo);
    
    if (error != 0) 
    {
        std::cout << "Error getaddrinfo: " << gai_strerror(error) << std::endl;
        return -1;
    }

    serverFd = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
    if (serverFd == -1) 
    {
        freeaddrinfo(serverInfo);
        printError(SOCKET);
        return -1;
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) 
    {
        freeaddrinfo(serverInfo);
        close(serverFd);
        printError(SETSOCKET);
        return -1;
    }

    if (bind(serverFd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) 
    {
        freeaddrinfo(serverInfo);
        close(serverFd);
        printError(BIND);
        return -1;
    }

    freeaddrinfo(serverInfo);
    if (listen(serverFd, 5) == -1) 
    {
        close(serverFd);
        printError(LISTEN);
        return -1;
    }
    return serverFd;
}

int createClientSocket(int fd)
{
    int clientFd;
    socklen_t clientSize;
    struct sockaddr_storage clientStruct;

    clientSize = sizeof(clientStruct);
    clientFd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize);
    if (clientFd == -1) {
        printError(ACCEPT);
        return -1;
    }
    return clientFd;
}

int Server::readData(int fd, std::string &str, int &bytes)
{
    char buffer[BUFFER];
    ft_memset(buffer, 0, sizeof(buffer));
    ssize_t res = recv(fd, buffer, BUFFER - 1, 0);
    if (res > 0) {
        buffer[res] = '\0';
        str.append(buffer, res);
        bytes += res;
        return 1;
    }
    return res;
}

bool Server::parseRequest(const char* str, int size, ClientRequest& request, 
                         std::map<std::string, std::string>& httpRequestLine) 
{
    request.parseRequestHttp(str, size);
    httpRequestLine = request.getRequestLine();
    return httpRequestLine.find("Method") != httpRequestLine.end();
}

void Server::handleCGIRequest(const ClientRequest& request, const std::string& uri, int fd, const InfoServer& info) 
{
    try 
    {
        CGI* cgi = new CGI(request, "/tmp/uploads", info);
        CGITracker tracker = {cgi, fd, cgi->getProcessedBody()};
        cgi_map[cgi->getInPipeWriteFd()] = tracker;
        cgi_map[cgi->getOutPipeReadFd()] = tracker;

        if (!tracker.input_data.empty()) 
        {
            struct pollfd in_fd = {cgi->getInPipeWriteFd(), POLLOUT, 0};
            poll_sets.push_back(in_fd);
        }
        struct pollfd out_fd = {cgi->getOutPipeReadFd(), POLLIN, 0};
        poll_sets.push_back(out_fd);

        std::cout << "CGI request queued for " << uri << " (" << request.getRequestLine()["Method"] << ") on client " << fd << std::endl;
    } 
    catch (const CGIException& e)
    {
        std::cerr << "CGI initialization failed for " << uri << ": " << e.what() << std::endl;
        std::string response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        queueResponse(fd, response, "");
    }
}

void Server::handleNonCGIRequest(const std::string& method, const std::string& uri, InfoServer info, 
                                 const ClientRequest& request, const char* str, int size, int fd) 
{
    ServerResponse serverResponse;
    std::string response;

    if (method == "GET")
        response = serverResponse.responseGetMethod(info, request);
    else if (method == "POST") 
        response = serverResponse.responsePostMethod(info, request, str, size);
    else if (method == "DELETE") 
        response = serverResponse.responseDeleteMethod(info, request);
    else 
    {
        std::cout << "Unsupported method: " << method << std::endl;
        response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    }
    if (!response.empty())
        queueResponse(fd, response, method);
}

void Server::queueResponse(int fd, const std::string& response, const std::string& method) 
{
    ResponseTracker tracker = {fd, response, method};
    response_queue.push_back(tracker);
    for (auto& pfd : poll_sets) 
    {
        if (pfd.fd == fd) 
        {
            pfd.events |= POLLOUT;
            break;
        }
    }
}

void Server::handleSendResponse(std::vector<struct pollfd>::iterator& it) 
{
    for (size_t i = 0; i < response_queue.size(); ++i) 
    {
        if (response_queue[i].fd == it->fd) {
            ssize_t sent = send(it->fd, response_queue[i].response.c_str(), 
                                response_queue[i].response.size(), 0);
            if (sent == static_cast<ssize_t>(response_queue[i].response.size())) 
            {
                std::cout << "Done with " << (response_queue[i].method.empty() ? "error" : response_queue[i].method) 
                          << " response for client " << it->fd << std::endl;
                response_queue.erase(response_queue.begin() + i);
                it->events &= ~POLLOUT;
                ++it;
            } 
            else if (sent < 0) 
            {
                std::cerr << "Failed to send response to client " << it->fd << std::endl;
                close(it->fd);
                for (size_t j = 0; j < client_trackers.size(); ++j) 
                {
                    if (client_trackers[j].fd == it->fd) 
                    {
                        client_trackers.erase(client_trackers.begin() + j);
                        break;
                    }
                }
                response_queue.erase(response_queue.begin() + i);
                it = poll_sets.erase(it);
            } 
            else 
            {
                response_queue[i].response = response_queue[i].response.substr(sent);
                ++it;
            }
            return;
        }
    }
    ++it;
}

void Server::serverParsingAndResponse(const char *str, InfoServer info, int fd, int size) 
{
    std::cout << "Parsing" << std::endl;
    ClientRequest request;
    std::map<std::string, std::string> httpRequestLine;

    try 
    {
        if (!parseRequest(str, size, request, httpRequestLine)) 
        {
            std::cout << "Method not found in HTTP request" << std::endl;
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            queueResponse(fd, response, "");
            return;
        }

        std::string method = httpRequestLine["Method"];
        std::string uri = httpRequestLine["Request-URI"];

        if (uri.find(".py") != std::string::npos && (method == "GET" || method == "POST"))
            handleCGIRequest(request, uri, fd, info);
        else 
            handleNonCGIRequest(method, uri, info, request, str, size, fd);
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error parsing request for client " << fd << ": " << e.what() << std::endl;
        std::string response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        queueResponse(fd, response, "");
    }
}

bool Server::isClientTimedOut(int fd) const 
{
    for (size_t i = 0; i < client_trackers.size(); ++i) 
    {
        if (client_trackers[i].fd == fd && (time(NULL) - client_trackers[i].start_time >= 5))
            return true;
    }
    return false;
}

void Server::initializeServerSockets(InfoServer info, int* arraySockets) 
{
    std::vector<std::string> ports = info.getArrayPorts();
    
    for (int i = 0; i < (int)ports.size() && i < 2; i++) 
    {
        arraySockets[i] = createServerSocket(ports[i].c_str());
        if (arraySockets[i] < 0) 
        {
            for (int j = 0; j < i; j++)
                close(arraySockets[j]);
            throw std::runtime_error("Failed to initialize server sockets");
        }
    }
}

void Server::addServerSocketToPoll(int fd)
{
    struct pollfd serverFd = {fd, POLLIN, 0};
    poll_sets.push_back(serverFd);
}

void Server::handleServerSocketEvent(int fd) 
{
    int clientSocket = createClientSocket(fd);
    if (clientSocket == -1) 
    {
        std::cerr << "Failed to accept client on socket " << fd << std::endl;
        return;
    }
    struct pollfd clientFd = {clientSocket, POLLIN, 0};
    poll_sets.push_back(clientFd);
    ClientTracker tracker = {clientSocket, time(NULL)};
    client_trackers.push_back(tracker);
    std::cout << "client created: " << clientSocket << std::endl;
}

void Server::handleCGIInput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it) 
{
    CGI* cgi = tracker.cgi;
    ssize_t written = write(it->fd, tracker.input_data.c_str() + cgi->_bytes_written,
                            tracker.input_data.size() - cgi->_bytes_written);
    
    if (written > 0) 
    {
        std::cout << "Wrote " << written << " bytes to CGI" << std::endl;
        cgi->updateBytesWritten(written);
    }
    else
        std::cerr << "write to CGI failed" << std::endl;
    
    if (cgi->_input_done) 
    {
        close(it->fd);
        it = poll_sets.erase(it);
    } 
    else
        ++it;
}

void Server::handleCGIOutput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it) 
{
    CGI* cgi = tracker.cgi;
    char buffer[1024];
    
    ssize_t bytes_read = read(it->fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) 
    {
        buffer[bytes_read] = '\0';
        std::cout << "Received " << bytes_read << " bytes from CGI: " << buffer << std::endl;
        cgi->appendOutput(buffer, bytes_read);
    } 
    else if (bytes_read == 0) 
    {
        std::cout << "CGI output complete (EOF)" << std::endl;
        cgi->_output_done = true;
    } 
    else
        std::cerr << "read from CGI failed" << std::endl;
    
    if (cgi->isDone() || cgi->isTimedOut())
        cleanupCGI(tracker, it);
    else
        ++it;
}

void Server::cleanupCGI(CGITracker& tracker, std::vector<struct pollfd>::iterator& it) 
{
    CGI* cgi = tracker.cgi;
    if (cgi->isTimedOut())
        std::cerr << "CGI process " << cgi->getPid() << " timed out" << std::endl;

    std::string response = cgi->getOutput();
    if (!response.empty())
        queueResponse(tracker.client_fd, response, "CGI");

    close(tracker.client_fd);
    close(it->fd);
    delete tracker.cgi;
    cgi_map.erase(cgi->getInPipeWriteFd());
    cgi_map.erase(it->fd);
    
    for (size_t i = 0; i < client_trackers.size(); ++i)
    {
        if (client_trackers[i].fd == tracker.client_fd) 
        {
            client_trackers.erase(client_trackers.begin() + i);
            break;
        }
    }
    it = poll_sets.erase(it);
}

void Server::handleClientSocketEvent(int fd, InfoServer info, std::vector<struct pollfd>::iterator& it) 
{
    if (isClientTimedOut(fd)) 
    {
        std::cout << "Client " << fd << " timed out" << std::endl;
        close(fd);
        for (size_t i = 0; i < client_trackers.size(); ++i) 
        {
            if (client_trackers[i].fd == fd) 
            {
                client_trackers.erase(client_trackers.begin() + i);
                break;
            }
        }
        it = poll_sets.erase(it);
        return;
    }

    std::string full_buffer;
    int totBytes = 0;
    std::cout << "Recv client request" << std::endl;
    int bytesRecv = readData(fd, full_buffer, totBytes);
    
    if (bytesRecv == 0 || bytesRecv == -1) 
    {
        std::cout << "Client " << fd << " disconnected" << std::endl;
        close(fd);
        for (size_t i = 0; i < client_trackers.size(); ++i) 
        {
            if (client_trackers[i].fd == fd) 
            {
                client_trackers.erase(client_trackers.begin() + i);
                break;
            }
        }
        it = poll_sets.erase(it);
    } 
    else if (bytesRecv == 1 && !full_buffer.empty()) 
    {
        serverParsingAndResponse(full_buffer.c_str(), info, fd, totBytes);
        ++it;
    } 
    else
        ++it;
}

void Server::setupServerSockets(InfoServer info, int* arraySockets)
{
    initializeServerSockets(info, arraySockets);
    for (int i = 0; i < 1; i++) // Single port for testing
    {  
        if (arraySockets[i] != -1)
            addServerSocketToPoll(arraySockets[i]);
    }
    std::cout << "Waiting connections..." << std::endl;
}

bool Server::runPollLoop(InfoServer info, int* arraySockets)
{
    std::cout << "Poll called" << std::endl;
    int returnPoll = poll(poll_sets.data(), poll_sets.size(), 1 * 60 * 1000);
    if (returnPoll == -1) 
    {
        std::cerr << "poll failed: " << strerror(errno) << std::endl;
        printError(POLL);
        return false;
    } 
    else if (returnPoll == 0) 
    {
        std::cout << "Waiting connections timeout 1 minute...closing server" << std::endl;
        return false;
    } 
    else 
    {
        dispatchEvents(info, arraySockets, returnPoll);
        return true;
    }
}

void Server::dispatchEvents(InfoServer info, int* arraySockets, int returnPoll)
{
    for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end();) 
    {
        if (it->revents & (POLLIN | POLLOUT)) 
        {
            if (it->fd == arraySockets[0] || it->fd == arraySockets[1])
                handleServerSocketEvent(it->fd);

            else if (cgi_map.find(it->fd) != cgi_map.end()) 
            {
                CGITracker& tracker = cgi_map[it->fd];
                if (it->revents & POLLOUT && it->fd == tracker.cgi->getInPipeWriteFd())
                    handleCGIInput(tracker, it);
                else if (it->revents & POLLIN && it->fd == tracker.cgi->getOutPipeReadFd())
                    handleCGIOutput(tracker, it); 
                else
                    ++it;
            } 
            else if (it->revents & POLLIN)
                handleClientSocketEvent(it->fd, info, it); 
            else if (it->revents & POLLOUT)
                handleSendResponse(it);
            else
                ++it;
        } 
        else
            ++it;
    }
}

void Server::startServer(InfoServer info) 
{
    int arraySockets[2] = {-1, -1};
    try 
    {
        setupServerSockets(info, arraySockets);
        while (runPollLoop(info, arraySockets)) 
        {
            // Continue looping until poll fails or times out
        }
    } 
    catch (const std::exception& e)
        std::cerr << "startServer failed: " << e.what() << std::endl;
    
    for (int i = 0; i < 2; i++)
        if (arraySockets[i] >= 0) close(arraySockets[i]);
}