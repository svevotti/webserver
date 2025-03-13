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


	public:
		std::vector<InfoServer*>	_servlist;
		Config( const std::string& configFile);
		Config( const Config& copy);
		Config& operator=(const Config& copy);
		~Config();
		bool	parseServer(std::istream &conf);
		bool	parseLocation(std::istream &conf, InfoServer *server, const std::string location);
		std::set<std::string>	parseMethods(std::string method_list);
		bool parseConfigFile(const std::string &configFile);
		bool	trimLine(std::string& line);
};

struct mylocations {
	std::vector<std::string>	methods;
};

#endif
