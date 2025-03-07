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
	this->_servlist = parseConfigFile(configFile);

}

std::set<std::string>	Config::parseMethods(std::string method_list)
{
	std::set<std::string>	methods;

	std::string	new_method;
	std::string	leftover;
	size_t pos = method_list.find(' ');
	new_method = method_list.substr(0, pos);
	leftover = method_list.substr(pos + 1, method_list.length() - new_method.length());
	methods.insert(new_method);
	pos = leftover.find(" ");
	while (pos != std::string::npos)
	{
		new_method = leftover.substr(0, pos);
		leftover = leftover.substr(pos + 1, leftover.length() - new_method.length());
		methods.insert(new_method);
		pos = leftover.find(" ");
	}
	return methods;
}

bool	Config::parseLocation(std::istream &conf, InfoServer &server, const std::string location)
{
	Route		route;
	std::string	line;
	std::string	key;
	std::string	value;

	route.uri = location;
	while (std::getline(conf, line))
	{
		std::cout << "Enters while loop: " << line << std::endl;
		//Trim lines and get rid of empty ones
		size_t	start = line.find_first_not_of(" \t");
		size_t	end = line.find_last_not_of(" \t");
		if (start == std::string::npos || end == std::string::npos)
			continue;
		line = line.substr(start, end - start + 1);
		if (line.empty() || line[0] == '#')
			continue;

		if (line == "}") //end of block
		{
			if (!route.uri.empty() && !route.path.empty()) //Add a uri != path?
			{
				server.setRoutes(route.uri, route);
				std::cout << "exits" << std::endl;
				return true;
			}
			return false;
		}
		else
		{
			size_t pos = line.find('\t');
			if ( pos != std::string::npos)
			{
				key = line.substr(0, pos);
				value = line.substr(pos + 1, line.length() - key.length()); //To avoid the ; in the end
				//Trim again
				key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t") + 1);
				value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t") - value.find_first_not_of(" \t"));
				if (key == "root")
				{
					route.path =(value + route.uri);
					std::cout << "Path is: " << route.path << std::endl;
				}
				else if (key == "allow")
				{
					route.methods = parseMethods(value);
				}
			}
			else if (line == "internal;")
				route.internal = true;
		}
	}
	return (true);
}

//Parses a block of code that starts with "server"
bool	Config::parseServer(std::istream &conf, InfoServer &server)
{
	std::string	line;
	std::string	key;
	std::string	value;
	bool		location_ok = false;

	while (std::getline(conf, line))
	{
		std::cout << "Enters while loop: " << line << std::endl;
		//Trim lines and get rid of empty ones
		size_t	start = line.find_first_not_of(" \t");
		size_t	end = line.find_last_not_of(" \t");
		if (start == std::string::npos || end == std::string::npos)
			continue;
		line = line.substr(start, end - start + 1);
		if (line.empty() || line[0] == '#')
			continue;

		//When we get to the end of the block or get to a location block, we return to the previous line and exit
		if (line.find("server ") != std::string::npos || line == "}")
		{
			std::cout << "tries to exit" << std::endl;
			conf.seekg(-(static_cast<int>(line.length()) + 1), std::ios::cur); //seekg: move the pointer conf length + 1 char back. ios::cur means current position
			if(!server._port.empty() && !server._ip.empty() && !server._root.empty() && server.isIPValid(server._ip))
				return true;
			return false;
		}
		else if (line.find("location ") != std::string::npos)
		{
			line = line.substr(line.find("location ") + 9);
			size_t n = line.find(" ");
			std::string	location = line.substr(0, n);
			location_ok = parseLocation(conf, server, location);
			if (location_ok == false)
			{
				std::cerr << "Error in location block!" << std::endl;
			}
			location_ok = false;
		}
		else
		{
			//We look for the tab(s) that split the key and value
			size_t pos = line.find('\t');
			key = line.substr(0, pos);
			value = line.substr(pos + 1, line.length() - key.length()); //To avoid the ; in the end
			//Trim again
			key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t") + 1);
			value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t") - value.find_first_not_of(" \t"));
			_settings[key] = value;
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
				if (server.isIPValid(value))
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
	}
	std::cout << "This should not have happened (parseServer)" << std::endl;
	return false; //We should never get here, so if we do, something is messed up somewhere
}


//Parses the config file and populates all the attributes of the Config class
std::vector<InfoServer>	Config::parseConfigFile(const std::string &configFile)
{
	std::vector<InfoServer>	server_list;
	std::string			line;
	std::ifstream 		conf(configFile.c_str());
	bool				started_server = false;

	//Checks if the file is accessible
	std::cout << "Opening file" <<std::endl;
	if (!conf)
	{
		std::cerr << "Error: Unable to open file " << configFile << std::endl;
		return server_list;
	}

	//reads line by line, extracts all parameters and location blocks
	InfoServer	new_server("", "", "", "");
	std::cout << "Reading file" <<std::endl;
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

		if (line.find("server ") != std::string::npos) //"[[server]]" means find server as a single word
		{
			started_server = parseServer(conf, new_server);
			if (started_server == false)
			{
				std::cout << "Something went wrong in parseServer" <<std::endl;
				return server_list;
			}
			server_list.push_back(new_server);
		}
	// 	//Look for info, locations have space separators, while parameters have \t
	// 	size_t pos = line.find('\t');
	// 	if (pos != std::string::npos) //if pos == npos, this is a line with a location block or some error, so we skip this loop
	// 	{
	// 		std::string key = line.substr(0, pos);
	// 		std::string value = line.substr(pos + 1, line.length() - 1); //To avoid the ; in the end
	// 		key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t"));
	// 		value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t"));
	// 		_settings[key] = value;
	// 	}
	// 	size_t pos = line.find(' ');
	// 	if (pos != std::string::npos) //if pos == npos, this is a line with a parameter (so already done) or error (so skip)
	// 	{
	// 		std::string location = line.substr(pos + 1, location.length() - 2);
	// 		mylocations location;
	// 		while (getline(conf, line))
	// 		{
	// 			start = line.find_first_not_of(" \t");
	// 			end = line.find_last_not_of(" \t");
	// 			if (line == "}")
	// 				continue;
	// 			//Need to parse all the arguments from the location block, store into a struct ?
	// 		}
	// 	}
	// }
	// _port = std::stoi(_settings["port"]);
	// _root = _settings["root_directory"];

	// // Get allowed methods and parse into vector methods
	// _allowed_methods = _settings["allowed_methods"];
	// std::stringstream ss(_allowed_methods);
	// std::string method;
	// while (getline(ss, method, ','))
	// 	_methods.push_back(method);

	// //Here I would run isValid() to check if all essential info is there. isValid should also add as default those that are missing and a default is available
	// _valid = true;
	}
	std::cout << "All good!" <<std::endl;
	return server_list;
}
