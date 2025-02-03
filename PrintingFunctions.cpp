#include "PrintingFunctions.hpp"
#include "StringManipulations.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>

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
	// printf("- buffer %s\n", buffer);
	// std::cout << "- appended string: " << full_str << std::endl;
	std::cout << "- content-length: " << size << std::endl;
	std::cout << "- message size so far: " << accumilating_size << std::endl;
	(void)buffer;
	(void)full_str;
}