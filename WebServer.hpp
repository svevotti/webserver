#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "StringManipulations.hpp"
#include "Logger.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

typedef struct client {
    int             fd;
    ClientRequest   request;
    std::string     response;
}              client;

class Webserver {
    public:
        Webserver(InfoServer&);
        ~Webserver();
        void	                                startServer();
        void                                    addServerSocketsToPoll(std::vector<int>);
        int                                     fdIsServerSocket(int);
        void                                    dispatchEvents();
        void                                    createNewClient(int);
        int                                     readData(int, std::string&, int&);
        int                                     handleReadEvents(int, std::vector<struct pollfd>::iterator);
        void                                    handleWritingEvents(int, std::vector<struct pollfd>::iterator);
        ClientRequest                           ParsingRequest(std::string, int);
        void                                    closeSockets();
        int                                     isCgi(std::string);
        int                                     searchPage(std::string path);
        std::string                             prepareResponse(ClientRequest);
        std::vector<struct client>::iterator    retrieveClient(int fd);

    private:

        InfoServer                  *_serverInfo;
        std::vector<struct pollfd>  poll_sets;
        std::vector<int>            serverFds;
        std::vector<struct client>  clientsQueue;

};
#endif