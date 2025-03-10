#ifndef CLIENT_H
#define CLIENT_H

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
#include <vector>

#include "StringManipulations.hpp"
#include "HttpRequest.hpp"
#include "Logger.hpp"
#include "ClientRequest.hpp"
#include "InfoServer.hpp"
#include "ServerResponse.hpp"
#define BUFFER 1024
#define DISCONNECTED 0
#define NODATA 1
#define CGI 3

class Client {
	public:
        Client(int, InfoServer);

        int     getFd();

        int     processClient();
        int     readData(int fd, std::string &str, int &bytes);
        int     isCgi(std::string str);
        int     searchPage(std::string path);
        std::string prepareResponse(ClientRequest request);
	private:
        ClientRequest request;
        std::string response;
        int         fd;
        InfoServer info;
};

#endif