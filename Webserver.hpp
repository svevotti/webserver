#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>
#include <dirent.h>
#include <cstring>
#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "InfoServer.hpp"
#include "ServerSockets.hpp"
#include "Logger.hpp"
#include "CGI.hpp"

//#define BUFFER 1024 // moved defs here? (more standard)
//#define MAX 100

typedef struct ClientTracker {
    int fd;
    ClientRequest request;
    std::string response;
} ClientTracker;

typedef struct CGITracker {
    CGI* cgi;
    int fd;// Current pipe FD (in_pipe[1] or out_pipe[0])
    int clientFd;// Added: Original client FD
    std::string input_data;
} CGITracker;

class Webserver {
    
    public:
                    Webserver(InfoServer&);
                    ~Webserver();
        void        startServer();
        void        addServerSocketsToPoll(std::vector<int>);
        int         fdIsServerSocket(int);
        void        dispatchEvents();
        void        handlePollInEvent(int fd);
        void        handlePollOutEvent(int fd);
        void        handlePollErrorOrHangup(int fd);
        void        handleCgiOutputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt); // New
        void        handleCgiInputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt);  // New
        void        handleClientError(int fd);                                                  // New
        void        createNewClient(int);
        int         readData(int, std::string&, int&);
        void        handleReadEvents(int);
        void        handleClientDisconnect(int fd, int bytesRecv);                              // New
        void        processRequest(int fd, int contentLength);                        // New
        void        setupCGI(int fd, std::vector<struct ClientTracker>::iterator& clientIt);    // New
        void        handleCgiOutput(int fd, std::vector<CGITracker>::iterator& cgiIt);          // New
        void        handleWritingEvents(int);
        void        writeCGIInput(int fd, std::vector<CGITracker>::iterator& cgiIt);            // New
        void        sendClientResponse(int fd, std::vector<struct ClientTracker>::iterator& iterClient); // New
        ClientRequest ParsingRequest(std::string, int);
        void        closeSockets();
        int         searchPage(std::string path);
        int         isCGI(std::string);
        std::string prepareResponse(ClientRequest);
        std::vector<struct ClientTracker>::iterator retrieveClient(int fd);
        std::vector<struct CGITracker>::iterator retrieveCGI(int fd);

    private:
        InfoServer *_serverInfo; // could we perhaps call the struct also serverInfo? Just  because if it has the same name as the class and it can be confusing. 
        std::vector<struct pollfd> poll_sets;
        std::vector<struct pollfd>::iterator it;
        std::vector<struct pollfd>::iterator end;
        int totBytes;
        std::string full_buffer;
        std::vector<int> serverFds;
        std::vector<struct ClientTracker> clientsQueue;
        std::vector<struct CGITracker> cgiQueue;
};

#endif