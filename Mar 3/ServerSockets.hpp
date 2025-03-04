#ifndef SERVER_SOCKETS_H
#define SERVER_SOCKETS_H

#include <sys/socket.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>
#include <netdb.h>

#include "InfoServer.hpp"

class ServerSockets 
{
    public:
    
        ServerSockets(InfoServer info);
        std::vector<int> getServerSockets() const;

    private:
        
        void initSockets(InfoServer info);
        int createSocket(const char* portNumber);
        std::vector<int> _serverFds;
        
};

#endif