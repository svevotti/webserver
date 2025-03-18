#include "Config.hpp"

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
		Route cgi_route;
		cgi_route = (*servIt)->getCGI();
		std::cout << "Checking CGI route " << std::endl << std::endl;
			std::cout << "URI: " << cgi_route.uri << std::endl;
			std::cout << "Path: " << cgi_route.path << std::endl;
			if (cgi_route.internal)
				std::cout << "It is internal!" << std::endl;
			else
				std::cout << "It is not internal!" << std::endl;
			std::cout << "Allowed methods: ";
			for(setIt = cgi_route.methods.begin(); setIt != cgi_route.methods.end(); setIt++)
				std::cout << (*setIt) << " ";
			std::cout << std::endl << "Other variables:" << std::endl;
			for(varIt = cgi_route.locSettings.begin(); varIt != cgi_route.locSettings.end(); varIt++)
				std::cout << varIt->first << " = " << varIt->second << std::endl;
			std::cout << std::endl;
	}
	return true;
}

int	main(int argc, char** argv){
	if (argc != 2)
	{
		std::cout << "Error! Please include only a path to a config file" << std::endl;
		return 1;
	}
		Config	configuration(argv[1]);
	std::cout << "Finished configuration!" << std::endl;
	if (test(configuration))
		std::cout << "All worked well!" << std::endl;
	else
		std::cout << "Error!" << std::endl;
	return 0;
}
