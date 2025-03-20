#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "HttpResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "StringManipulations.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"

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

typedef struct client {
    int             fd;
    HttpRequest   request;
    std::string     response;
    std::string     raw_data;
    int             totbytes;
}              client;

typedef struct response {
    int             code;
    std::string     body;
}              response;

class Webserver {
    public:
        Webserver(InfoServer&);
        ~Webserver();
        int	                                    startServer();
        void                                    addServerSocketsToPoll(std::vector<int> vec);
        int                                     fdIsServerSocket(int fd);
        void                                    dispatchEvents(void);
        void                                    createNewClient(int fd);
        int                                     readData(int fd, std::string& buffer, int& bytes);
        int                                     handleReadEvents(int fd, std::vector<struct pollfd>::iterator it);
        void                                    handleWritingEvents(int fd, std::vector<struct pollfd>::iterator it);
        HttpRequest                             ParsingRequest(std::string buffer, int size);
        void                                    closeSockets(void);
        int                                     isCgi(std::string path);
        int                                     searchPage(std::string path);
        std::string                             prepareResponse(HttpRequest request);
        std::vector<struct client>::iterator    retrieveClient(int fd);

        std::string                                    retrievePage(HttpRequest request);
        std::string                                       uploadFile(HttpRequest request);
        std::string                                          deleteFile(HttpRequest request);

    private:

        InfoServer                  serverInfo;
        std::vector<struct pollfd>  poll_sets;
        std::vector<int>            serverFds;
        std::vector<struct client>  clientsQueue;

};
#endif