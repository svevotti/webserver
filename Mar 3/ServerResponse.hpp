#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

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
#include <map>
#include <cstdio>

#include "ClientRequest.hpp"
#include "ServerStatusCode.hpp"
#include "InfoServer.hpp"


class ServerResponse
{

    public:

        std::string responseGetMethod(InfoServer, ClientRequest);
        std::string responsePostMethod(InfoServer, ClientRequest);
        std::string responseDeleteMethod(InfoServer info, ClientRequest);

};

void printError(int error);

#endif