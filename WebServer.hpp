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
#include "ServerStatusCode.hpp"
#include "StringManipulations.hpp"
#include "PrintingFunctions.hpp"

class Webserver
{
public:
    void	startServer(InfoServer);
    void    addServerSocketsToPoll(std::vector<int>);
    void    dispatchEvents(InfoServer, std::vector<int>);
    int     createNewClient(int);
    void    ReadClientRequest(int, InfoServer);
    int     readData(int, std::string&, int&);
    void    handleReadEvents(int, InfoServer);

private:

    std::vector<struct pollfd> poll_sets;
    std::vector<struct pollfd>::iterator it;
	std::vector<struct pollfd>::iterator end;
    int totBytes;
    std::string full_buffer;
};
#endif