#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "HttpResponse.hpp"
#include "ServerSockets.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"
#include "ClientHandler.hpp"
#include "Config.hpp"
#include "CGIHandler.hpp" // Simona added this include

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
        Webserver(Config &file);
        ~Webserver();
        int                                     startServer(void);
        void                                    addServerSocketsToPoll(int fd);
        int                                     fdIsServerSocket(int fd);
        int                                     fdIsCGI(int fd);
        void                                    dispatchEvents(void);
        void                                    createNewClient(int fd);
        int                                     processClient(int fd, int event);
        void                                    closeSockets(void);
        std::vector<ClientHandler>::iterator    retrieveClient(int fd);
        // OLD VERSION (commented out by Simona) -- no return
         void                                    removeClient(std::vector<struct pollfd>::iterator it);
        // NEW VERSION -- with return
        //std::vector<struct pollfd>::iterator removeClient(std::vector<struct pollfd>::iterator it); // This way Simona can use it too for CGI stuff
        void checkTime(void);

    private:
        std::vector<InfoServer*> configInfo;
        InfoServer configServer;
        std::vector<struct pollfd>  poll_sets;
        int            serverFd;
        std::vector<ClientHandler>  clientsList;
        CGIHandler cgiHandler; // Added by Simona -- has to be here for processClient to use it

};
#endif