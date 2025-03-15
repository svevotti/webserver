#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

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
#include "Utils.hpp"

#define BUFFER 1024
#define DISCONNECTED 0
#define NODATA 1
#define READ 2
#define CGI 3

class ClientHandler {
	public:
                ClientHandler(int, InfoServer);

                int     getFd() const;
                ClientRequest getRequest() const;
                std::string  getResponse() const;


                int     receiveRequest();
                int     readData(int fd, std::string &str, int &bytes);
                int     isCgi(std::string str);
                int     searchPage(std::string path);
                std::string prepareResponse(ClientRequest request);
                int        sendResponse();

	private:
                ClientRequest request;
                std::string response;
                int         fd;
                InfoServer info;
};

std::ostream &operator<<(std::ostream &output, ClientHandler const &obj);

#endif