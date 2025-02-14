#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSocket.hpp"
#include "InfoServer.hpp"

int main(void)
{
	InfoServer		server;	
	Server 	sockets;

	server.setArrayPorts("8080");
	server.setArrayPorts("9090");
	server.setServerRootPath("/home/smazzari/repos/Github/Circle5/webserver/server_root/"); //folder
	server.setServerDocumentRoot("/home/smazzari/repos/Github/Circle5/webserver/server_root/public_html/"); //folder
	// server.setServerRootPath("/Users/sveva/repos/Circle5/webserver/server_root/public/"); //test on my laptop
	std::cout << server.getServerDocumentRoot();
	sockets.startServer(server);
	return (0);
}