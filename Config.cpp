#include "Config.hpp"

//default constructor (only for orthodox form)
Config::Config( void ) {
	parseConfigFile("default.conf");
}

//Copy constructor
Config::Config( const Config& copy) {
	*this = copy;
}

//Equal operator
Config&	Config::operator=( const Config& copy) {
	if (this != &copy)
	{
		return (*this);
	}

	return (*this);
}

//Deconstructor
Config::~Config() {
}

//Constructor that takes a configFile as argument
Config::Config( const std::string& configFile) {
	parseConfigFile(configFile);
}

//Parses a block of code that starts with "server"
bool	Config::parseServer(std::istream &conf, Server &server)
{
	std::string	line;
	std::string	key;
	std::string	value;

	while (std::getline(conf, line))
	{
		//Trim lines and get rid of empty ones
		size_t	start = line.find_first_not_of(" \t");
		size_t	end = line.find_last_not_of(" \t");
		if (start == std::string::npos || end == std::string::npos)
			continue;
		line = line.substr(start, end - start + 1);
		if (line.empty() || line[0] == '#')
			continue;

		//When we get to the end of the block or get to a location block, we return to the previous line and exit
		if (line.find("[[server]]") != std::string::npos || line.find("[[location]]") != std::string::npos || line == "}")
		{
			conf.seekg(-(static_cast<int>(line.length()) + 1), std::ios::cur); //seekg: move the pointer conf length + 1 char back. ios::cur means current position
			if(!server._port.empty() && !server._ip.empty() && !server._root.empty() && isIPValid(server._ip))
				return true;
			return false;
		}

		//We look for the tab(s) that split the key and value
		size_t pos = line.find('\t');
		key = line.substr(0, pos);
		value = line.substr(pos + 1, line.length() - 1); //To avoid the ; in the end
		//Trim again
		key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t"));
		value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t"));
		//Note: Think on potential problems in syntax, eg symbols? quotations? spaces and tabs? Make a checking function
		if (key == "port")
		{
			if(atoi(value.c_str()) > 0 && atoi(value.c_str()) < 65535)
				server.setPort(value); //Saved as string, change to int?
			else
				std::cerr << "Incorrect port value: " << value << std::endl;
		}
		if (key == "server_name") //host?
		{
			if (isIPValid(value))
				server.setIP(value); //Saved as string
			else
				std::cerr << "Incorrect IP: " << value << std::endl;
		}
		if (key == "root")
			server.setRoot(value);
		if (key == "index")
			server.setIndex(value);
		//Expand if we need more variables
	}
	return false; //We should never get here, so if we do, something is messed up somewhere
}


//Parses the config file and populates all the attributes of the Config class
std::vector<Server>	Config::parseConfigFile(const std::string &configFile)
{
	std::vector<Server>	server_list;
	std::string			line;
	std::ifstream 		conf(configFile);
	bool				finished_server = false;
	bool				finished_location = false;
	bool				finished_error = false;
	bool				started_server = false;

	//Checks if the file is accessible
	if (!conf)
	{
		std::cerr << "Error: Unable to open file " << configFile << std::endl;
		return server_list;
	}

	//reads line by line, extracts all parameters and location blocks
	Server	new_server(0, "", "", "");
	while (std::getline(conf, line))
	{
		//Trim lines and get rid of empty ones
		size_t	start = line.find_first_not_of(" \t");
		size_t	end = line.find_last_not_of(" \t");
		if (start == std::string::npos || end == std::string::npos)
			continue;
		line = line.substr(start, end - start + 1);
		if (line.empty() || line[0] == '#')
			continue;

		if (line.find("[[server]]") != std::string::npos) //"[[server]]" means find server as a single word
		{
			started_server = parseServer(conf, new_server);
		}
		//Look for info, locations have space separators, while parameters have \t
		size_t pos = line.find('\t');
		if (pos != std::string::npos) //if pos == npos, this is a line with a location block or some error, so we skip this loop
		{
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1, line.length() - 1); //To avoid the ; in the end
			key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t"));
			value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t"));
			_settings[key] = value;
		}
		size_t pos = line.find(' ');
		if (pos != std::string::npos) //if pos == npos, this is a line with a parameter (so already done) or error (so skip)
		{
			std::string location = line.substr(pos + 1, location.length() - 2);
			mylocations location;
			while (getline(conf, line))
			{
				start = line.find_first_not_of(" \t");
				end = line.find_last_not_of(" \t");
				if (line == "}")
					continue;
				//Need to parse all the arguments from the location block, store into a struct ?
			}
		}
	}
	_port = std::stoi(_settings["port"]);
	_root = _settings["root_directory"];

	// Get allowed methods and parse into vector methods
	_allowed_methods = _settings["allowed_methods"];
	std::stringstream ss(_allowed_methods);
	std::string method;
	while (getline(ss, method, ','))
		_methods.push_back(method);

	//Here I would run isValid() to check if all essential info is there. isValid should also add as default those that are missing and a default is available
	_valid = true;
}
