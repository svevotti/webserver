#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SocketServer.hpp"
#include "InfoServer.hpp"

int main(void)
{
	InfoServer		server;	
	SocketServer 	sockets;

	server.setArrayPorts("8080");
	server.setArrayPorts("9090");
	// server.setServerRootPath("/home/smazzari/repos/Github/Circle5/webserver/server_root/public/");
	server.setServerRootPath("/Users/sveva/repos/Circle5/webserver/server_root/public/"); //test on my laptop
	sockets.startSocket(server);
	return (0);
}
