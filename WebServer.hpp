#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>

#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "StringManipulations.hpp"
#include "PrintingFunctions.hpp"

typedef struct client
{
    int fd;
    std::string response;
} client;

class Webserver
{
public:
                Webserver(InfoServer&);
                ~Webserver();
    void	    startServer();
    void        addServerSocketsToPoll(std::vector<int>);
    int         fdIsServerSocket(int);
    void        dispatchEvents();
    int         createNewClient(int);
    int         readData(int, std::string&, int&);
    void        handleReadEvents(int);
    void        handleWritingEvents(int);
    ClientRequest        *ParsingRequest(std::string, int);
    std::string getFullBuffer() const;
    void        closeSockets();
    int        readInChunks(int, std::string&, int&);
    int         isCgi(std::string);
    int         searchPage(std::string path);
    std::string prepareResponse(ClientRequest *request);

private:

    InfoServer *_serverInfo;
    std::vector<struct pollfd> poll_sets;
    std::vector<struct pollfd>::iterator it;
	std::vector<struct pollfd>::iterator end;
    int totBytes;
    std::string full_buffer;
    ClientRequest   *request;
    std::vector<int>    serverFds;
    std::vector<struct client> clientsQueue;

};
#endif