#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSockets.hpp"
#include "InfoServer.hpp"
#include "WebServer.hpp"
#include "Config.hpp"
#include <signal.h>

int main(void)
{
	Config	configuration("default.conf");
	InfoServer infoServer;
	Webserver 	server(configuration, infoServer);

	if (server.startServer() == -1)
		Logger::error("Could not start the server");
	return (0);
}