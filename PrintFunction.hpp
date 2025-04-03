#ifndef PRINTFUNCTION_H
#define PRINTFUNCTION_H
#include <iostream>
#include <string>
#include "InfoServer.hpp"

void printRoute(const Route& route)
{
    std::cout << "Route Information:" << std::endl;
    std::cout << "URI: " << route.uri << std::endl;
    std::cout << "Path: " << route.path << std::endl;

    std::cout << "Methods: ";
    if (route.methods.empty()) {
        std::cout << "None";
    } else {
        for (std::set<std::string>::const_iterator it = route.methods.begin(); it != route.methods.end(); ++it) {
            std::cout << *it << " ";
        }
    }
    std::cout << std::endl;

	std::cout << "Location Settings:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = route.locSettings.begin(); it != route.locSettings.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cout << "Internal: " << (route.internal ? "true" : "false") << std::endl;
}

void printServInfo(InfoServer *server)
{
	std::map<std::string, std::string>::iterator	mapIt;
	std::map<std::string, Route>::iterator			locMetIt;
	std::set<std::string>::iterator					setIt;
	std::map<std::string, std::string>::iterator	varIt;
	std::cout << "Checking server " << std::endl;
	std::cout << "Checking harcoded variables" << std::endl << std::endl;
	if (server->getIP() != "")
		std::cout << "IP is " << server->getIP() <<std::endl;
	else
		std::cout << "IP doesn't exist!" << std::endl;
	if (server->getIndex() != "")
		std::cout << "Index is " << server->getIndex() <<std::endl;
	else
		std::cout << "Index doesn't exist!" << std::endl;
	if (server->getRoot() != "")
		std::cout << "Root is " << server->getRoot() <<std::endl;
	else
		std::cout << "Root doesn't exist!" << std::endl;
	if (server->getPort() != "")
		std::cout << "Port is " << server->getPort() <<std::endl;
	else
		std::cout << "Port doesn't exist!" << std::endl;

	std::cout << std::endl << "Checking all the settings" << std::endl << std::endl;

	std::map<std::string, std::string> map;

	map = server->getSetting();
	for(mapIt = map.begin(); mapIt != map.end(); mapIt++)
		std::cout << mapIt->first << " = " << mapIt->second << std::endl;

	std::cout << std::endl << "Checking all locations" << std::endl << std::endl;

	std::map<std::string, Route> routes;

	routes = server->getRoute();
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
}

#endif