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
	InfoServer		info;

	info.setArrayPorts("8080");
	info.setArrayPorts("9090");
	info.setServerNumber(2);
	// server.setServerRootPath("/home/smazzari/repos/Github/Circle5/webserver/server_root"); //folder
	// server.setServerDocumentRoot("/home/smazzari/repos/Github/Circle5/webserver/server_root/public_html"); //folder
	info.setServerRootPath("/Users/sveva/repos/Circle5/webserver/server_root"); //test on my laptop
	info.setServerDocumentRoot("/Users/sveva/repos/Circle5/webserver/server_root/public_html");
	//std::cout << server.getServerDocumentRoot();
	Webserver 	server(info);

	printf("before starting server\n");
	server.startServer(info);
	server.closeSockets();
	return (0);
}