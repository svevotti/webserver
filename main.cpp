#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSockets.hpp"
#include "InfoServer.hpp"
#include "WebServer.hpp"
#include <signal.h>

int main(void)
{
	InfoServer		info;
	Webserver 	server(info);

	if (server.startServer() == -1)
		Logger::error("Could not start the server");
	return (0);
}