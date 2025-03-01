#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSockets.hpp"
#include "InfoServer.hpp"
#include "WebServer.hpp"

int main(void)
{
	InfoServer		server;	
	Webserver 	webserver;

	server.setArrayPorts("8080");
	server.setArrayPorts("9090");
	server.setServerNumber(2);
	server.setServerRootPath("/home/smazzari/repos/Github/Circle5/webserver/server_root"); //folder
	server.setServerDocumentRoot("/home/smazzari/repos/Github/Circle5/webserver/server_root/public_html"); //folder
	// server.setServerRootPath("/Users/sveva/repos/Circle5/webserver/server_root"); //test on my laptop
	// server.setServerDocumentRoot("/Users/sveva/repos/Circle5/webserver/server_root/public_html");
	//std::cout << server.getServerDocumentRoot();
	printf("before starting server\n");
	webserver.startServer(server);
	return (0);
}