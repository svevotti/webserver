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
		std::vector<InfoServer*>	_servlist;
		bool parseConfigFile(const std::string &configFile);
		bool	parseServer(std::istream &conf);
		bool	parseLocation(std::istream &conf, InfoServer *server, const std::string location);
		bool	trimLine(std::string& line);

	public:
		Config( const std::string& configFile);
		Config( const Config& copy);
		Config& operator=(const Config& copy);
		~Config();
		std::set<std::string>	parseMethods(std::string method_list);

		void	setServerList( const std::vector<InfoServer*> servlist );
		std::vector<InfoServer*>	getServList( void ) const;
};

struct mylocations {
	std::vector<std::string>	methods;
};

#endif
