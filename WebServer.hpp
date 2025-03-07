#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "WebServer.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>
#include <dirent.h>

#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "StringManipulations.hpp"
#include "Logger.hpp"

typedef struct client
{
    int fd;
    ClientRequest request;
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
    void         createNewClient(int);
    int         readData(int, std::string&, int&);
    void        handleReadEvents(int, std::vector<struct pollfd>::iterator, int*);
    void        handleWritingEvents(int, std::vector<struct pollfd>::iterator);
    ClientRequest        ParsingRequest(std::string, int);
    void        closeSockets();
    int        readInChunks(int, std::string&, int&);
    int         isCgi(std::string);
    int         searchPage(std::string path);
    std::string prepareResponse(ClientRequest);
    std::vector<struct client>::iterator retrieveClient(int fd);

private:

    InfoServer *_serverInfo;
    std::vector<struct pollfd> poll_sets;
    int totBytes;
    std::string full_buffer;
    std::vector<int>    serverFds;
    std::vector<struct client> clientsQueue;

};
#endif