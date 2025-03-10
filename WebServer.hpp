#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "StringManipulations.hpp"
#include "Logger.hpp"
#include "ClientHandler.hpp"

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

#define TIMEOUT 2000

typedef struct client {
    int             fd;
    ClientRequest   request;
    std::string     response;
}              client;

class Webserver {
    public:
        Webserver(InfoServer&);
        ~Webserver();
        void    startServer();
        void    addServerSocketsToPoll(std::vector<int>);
        int     fdIsServerSocket(int);
        void    dispatchEvents();
        void    createNewClient(int);
        int     processClient(int fd);
        void    closeSockets();
        int     retrieveClientIndex(int fd);
        void    cleanupClient(int fd);

    private:

        InfoServer                  *serverInfo;
        std::vector<struct pollfd>  poll_sets;
        std::vector<int>            serverFds;
        std::vector<class ClientHandler> clientList;

};
#endif