#include "PrintingFunctions.hpp"
#include "StringManipulations.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

typedef struct header
{
	std::map<std::string, std::string> myMap;
	// std::string value;
} header;

void printIpvVersion(struct sockaddr_storage address)
{
	char s[256];
	//just checking wich IPV is - so i can print address properly
	if (address.ss_family == AF_INET)
	{
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)&address;
		inet_ntop(address.ss_family, sin, s, sizeof(s));
		std::cout << "server: got connection from " << s << std::endl;
	}
	else if (address.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *sin;
		sin = (struct sockaddr_in6 *)&address;
		inet_ntop(address.ss_family, sin, s, sizeof(s));
		std::cout << "server: got connection from " << s << std::endl;
	}
	else
		std::cout <<"Unknown" << std::endl;
	ft_memset(&s, 0, sizeof(s));
}

void printError(int error)
{
	switch (error)
	{
		case -1:
		std::cerr << "Error creating socket" << std::endl;
		break ;
		case -2:
		std::cerr << "Error getaddrinfo" << std::endl;
		break ;
		case -3:
		std::cerr << "Error fcntl" << std::endl;
		break ;
		case -4:
		std::cerr << "Error setsocket" << std::endl;
		break ;
		case -5:
		std::cerr << "Error bind" << std::endl;
		break ;
		case -6:
		std::cerr << "Error listen" << std::endl;
		break;
		case -7:
		std::cerr << "Error receive" << std::endl;
		break;
		case -8:
		std::cerr << "Error send" << std::endl;
		break;
		default:
		case -9:
		std::cerr << "Error accept" << std::endl;
		break;
		case -10:
		std::cerr << "Error poll" << std::endl;
		break;
		std::cerr << "Some other error" << std::endl;
	}
}

void infoRecvLoop(int number, int bytes, char *buffer, std::string full_str, int size, int accumilating_size)
{
	std::cout << "recv call number: " << number << std::endl;
	std::cout << "- received bytes: " << bytes << std::endl;
	std::cout << "- content-length: " << size << std::endl;
	std::cout << "- message size so far: " << accumilating_size << std::endl;
	(void)buffer;
	(void)full_str;
}

void printFcntlFlag(int fd)
{
	int flags = fcntl(fd, F_GETFL); // Get the current flags
	if (flags & O_NONBLOCK)
		printf("Socket correctly set to non-blocking\n");
	else 
		printf("Socket is not set to non-blocking\n");
}

	// std::map<int, struct header>::iterator outerIt;
		// std::map<std::string, std::string>::iterator innerIt;
		// // int i = 0;
		// for (outerIt = bodySections.begin(); outerIt != bodySections.end(); outerIt++)
		// {
		// 	// printf("i %d\n", i++);
		// 	// std::cout << "Index: " << outerIt->first << std::endl;
		// 	struct header section = outerIt->second;

		// 	for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
		// 	{
		// 		// std::cout << "index in vector: " << binaryDataIndex[0] << std::endl;
		// 		std::cout << innerIt->first << std::endl;
		// 		std::cout << innerIt->second << std::endl;
		// 	}
		// }

int printRecvFlag(int error)
{
	switch (error) {
			case EWOULDBLOCK:
				printf("No data available (non-blocking mode)\n");
				return 1;
			case ETIMEDOUT:
				printf("Receive operation timed out\n");
				break;
			case ECONNRESET:
				printf("Connection reset by peer\n");
				break;
			default:
				printf("recv error: %s\n", strerror(errno));
				break;
		}
	return 0;
}

void printReqHttpMap(std::map<std::string, std::string> myMap)
{
	std::map<std::string, std::string>::iterator element;
	std::map<std::string, std::string>::iterator ite = myMap.end();

	std::cout << "parsed item\n";
	for(element = myMap.begin(); element != ite; element++)
	{
		std::cout << element->first << " : " <<element->second << std::endl;
	}
}