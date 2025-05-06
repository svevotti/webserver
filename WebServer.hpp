#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "HttpResponse.hpp"
#include "ServerSockets.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"
#include "ClientHandler.hpp"
#include "Config.hpp"
#include "CGI.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#define DISCONNECTED 1
#define STATIC 2
#define CGI 3
#define READ 4
#define WRITE 5

class Webserver {
    public:
        pid_t pid;
        Webserver(Config &file);
        ~Webserver();
        int                                     startServer(void);
        void                                    addServerSocketsToPoll(int fd);
        int                                     fdIsServerSocket(int fd);
        bool                                     fdIsCGI(int fd);
        void                                    dispatchEvents(void);
        void                                    createNewClient(int fd);
        int                                     processClient(int fd, int event);
        int                                     processCGI(int fd);
        void                                    closeSockets(void);
        std::vector<ClientHandler>::iterator    retrieveClient(int fd);
        std::vector<ClientHandler>::iterator    retrieveClientCGI(int fd);
        void                                    removeClient(std::vector<struct pollfd>::iterator it);
        void timeoutResponse(int fd);
        void checkTime(void);
        InfoServer*	matchFD( int fd );

    private:
        std::vector<InfoServer*>    configInfo;
        std::vector<struct pollfd>  poll_sets;
        int                         serverFd;
        std::vector<ClientHandler>  clientsList;
        std::vector<CGITracker>     _cgiQueue;

};
#endif
