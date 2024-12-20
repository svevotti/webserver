#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SocketServer.hpp"

int main(void)
{

	SocketServer singleSocket;

	singleSocket.startSocket();
	return (0);
}
