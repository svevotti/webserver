#ifndef CONFIG_H
# define CONFIG_H

# include <iostream>
# include <map>
# include <vector>
# include <fstream>
# include <sstream>
# include <map>
# include <cstdlib>
# include <string>

class Config {
	private:
		Config( void );
		std::vector<Server>	parseConfigFile(const std::string &configFile);

		std::map<std::string, std::string>	_settings;
		bool	_valid = false;

		int	_port;
		std::string	_root;

	public:
		Config( const std::string& configFile);
		Config( const Config& copy);
		Config& operator=(const Config& copy);
		~Config();
};

struct mylocations {
	std::vector<std::string>	methods;
};

#endif
