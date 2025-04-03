#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSockets.hpp"
#include "WebServer.hpp"
#include "Config.hpp"
#include <signal.h>

bool	test(Config &conf)
{
	std::vector<InfoServer*>::iterator				servIt;
	std::map<std::string, std::string>::iterator	mapIt;
	std::map<std::string, Route>::iterator			locMetIt;
	std::set<std::string>::iterator					setIt;
	std::map<std::string, std::string>::iterator	varIt;
	int	i;

	i = 0;
	std::vector<InfoServer*> server;
	server = conf.getServList();
	if (server.size() == 0)
		return false;
	for(servIt = server.begin(); servIt != server.end(); servIt++)
	{
		i++;
		std::cout << "Checking server " << i << std::endl << std::endl;
		std::cout << "Checking harcoded variables" << std::endl << std::endl;
		if ((*servIt)->getIP() != "")
			std::cout << "IP is " << (*servIt)->getIP() <<std::endl;
		else
			std::cout << "IP doesn't exist!" << std::endl;
		if ((*servIt)->getIndex() != "")
			std::cout << "Index is " << (*servIt)->getIndex() <<std::endl;
		else
			std::cout << "Index doesn't exist!" << std::endl;
		if ((*servIt)->getRoot() != "")
			std::cout << "Root is " << (*servIt)->getRoot() <<std::endl;
		else
			std::cout << "Root doesn't exist!" << std::endl;
		if ((*servIt)->getPort() != "")
			std::cout << "Port is " << (*servIt)->getPort() <<std::endl;
		else
			std::cout << "Port doesn't exist!" << std::endl;

		std::cout << std::endl << "Checking all the settings" << std::endl << std::endl;

		std::map<std::string, std::string> map;

		map = (*servIt)->getSetting();
		for(mapIt = map.begin(); mapIt != map.end(); mapIt++)
			std::cout << mapIt->first << " = " << mapIt->second << std::endl;

		std::cout << std::endl << "Checking all locations" << std::endl << std::endl;

		std::map<std::string, Route> routes;

		routes = (*servIt)->getRoute();
		for(locMetIt = routes.begin(); locMetIt != routes.end(); locMetIt++)
		{
			std::cout << "Checking location " << locMetIt->first << std::endl << std::endl;
			std::cout << "URI: " << locMetIt->second.uri << std::endl;
			std::cout << "Path: " << locMetIt->second.path << std::endl;
			if (locMetIt->second.internal)
				std::cout << "It is internal!" << std::endl;
			else
				std::cout << "It is not internal!" << std::endl;
			std::cout << "Allowed methods: ";
			for(setIt = locMetIt->second.methods.begin(); setIt != locMetIt->second.methods.end(); setIt++)
				std::cout << (*setIt) << " ";
			std::cout << std::endl << "Other variables:" << std::endl;
			for(varIt = locMetIt->second.locSettings.begin(); varIt != locMetIt->second.locSettings.end(); varIt++)
				std::cout << varIt->first << " = " << varIt->second << std::endl;
			std::cout << std::endl;
		}
		// Route cgi_route;
		// cgi_route = (*servIt)->getCGI();
		// std::cout << "Checking CGI route " << std::endl << std::endl;
		// 	std::cout << "URI: " << cgi_route.uri << std::endl;
		// 	std::cout << "Path: " << cgi_route.path << std::endl;
		// 	if (cgi_route.internal)
		// 		std::cout << "It is internal!" << std::endl;
		// 	else
		// 		std::cout << "It is not internal!" << std::endl;
		// 	std::cout << "Allowed methods: ";
		// 	for(setIt = cgi_route.methods.begin(); setIt != cgi_route.methods.end(); setIt++)
		// 		std::cout << (*setIt) << " ";
		// 	std::cout << std::endl << "Other variables:" << std::endl;
		// 	for(varIt = cgi_route.locSettings.begin(); varIt != cgi_route.locSettings.end(); varIt++)
		// 		std::cout << varIt->first << " = " << varIt->second << std::endl;
		// 	std::cout << std::endl;
	}
	return true;
}

int main(void)
{
	Config	configuration("default.conf");

	// test(configuration);
	if (configuration.ft_validServer())
	{
		Webserver 	server(configuration);
		if (server.startServer() == -1)
		{
			Logger::error("Could not start the server");
			return 1;
		}
	}
	else
		Logger::error("Parsing configuration file");
	return (0);
}
