#ifndef CONFIG_H
# define CONFIG_H

# include <iostream>
# include <cstdlib>
# include <map>
# include <vector>
# include <fstream>
# include <sstream>
# include <map>
# include <string>
# include "InfoServer.hpp"

class Config {
	private:
		Config( void );

		std::map<std::string, std::string>	_settings;

		std::string	_root;

	public:
		std::vector<InfoServer>	_servlist;
		Config( const std::string& configFile);
		Config( const Config& copy);
		Config& operator=(const Config& copy);
		~Config();
		bool	parseServer(std::istream &conf, InfoServer &server);
		bool	parseLocation(std::istream &conf, InfoServer &server, const std::string location);
		std::set<std::string>	parseMethods(std::string method_list);
		std::vector<InfoServer> parseConfigFile(const std::string &configFile);
};

struct mylocations {
	std::vector<std::string>	methods;
};

#endif
