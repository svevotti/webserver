#include "Config.hpp"

bool	test(Config &conf)
{
	std::vector<InfoServer*>::iterator				servIt;
	std::map<std::string, std::string>::iterator	mapIt;
	std::map<std::string, Route>::iterator			locMetIt;
	std::set<std::string>::iterator					setIt;
	int	i;

	i = 0;
	for(servIt = conf._servlist.begin(); servIt != conf._servlist.end(); servIt++)
	{
		i++;
		std::cout << "Checking server " << i << std::endl << std::endl;
		std::cout << "Checking harcoded variables" << std::endl << std::endl;
		if ((*servIt)->_ip != "")
			std::cout << "IP is " << (*servIt)->_ip <<std::endl;
		else
			std::cout << "IP doesn't exist!" << std::endl;
		if ((*servIt)->_index != "")
			std::cout << "Index is " << (*servIt)->_index <<std::endl;
		else
			std::cout << "Index doesn't exist!" << std::endl;
		if ((*servIt)->_root != "")
			std::cout << "Root is " << (*servIt)->_root <<std::endl;
		else
			std::cout << "Root doesn't exist!" << std::endl;
		if ((*servIt)->_port != "")
			std::cout << "Port is " << (*servIt)->_port <<std::endl;
		else
			std::cout << "Port doesn't exist!" << std::endl;

		std::cout << std::endl << "Checking all the settings" << std::endl << std::endl;
		for(mapIt = (*servIt)->_settings.begin(); mapIt != (*servIt)->_settings.end(); mapIt++)
			std::cout << mapIt->first << ":" << mapIt->second << std::endl;
		std::cout << std::endl << "Checking all locations" << std::endl << std::endl;
		for(locMetIt = (*servIt)->_routes.begin(); locMetIt != (*servIt)->_routes.end(); locMetIt++)
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
			std::cout << std::endl << std::endl;;
		}
	}
	std::cout << conf._servlist[0]->_index <<std::endl;
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
	std::cout << configuration._servlist.at(0)->_ip << std::endl;
	if (test(configuration))
		std::cout << "All worked well!" << std::endl;
	else
		std::cout << "Error!" << std::endl;
	return 0;
}
