#include "Config.hpp"

//default constructor (only for orthodox form)
//Copy constructor
// Config::Config( const Config& copy) {
// 	*this = copy;
// }

//Equal operator
Config&	Config::operator=( const Config& copy) {
	if (this != &copy)
	{
		_servcount = copy.getServCount();
		std::vector<InfoServer*> server;
		server = copy.getServList();
		std::vector<InfoServer*>::iterator				servIt;
		for (servIt = server.begin(); servIt != server.end(); servIt++)
		{
			InfoServer *new_server= new InfoServer[_servcount];
			new_server = (*servIt);
			_servlist.push_back(new_server);
		}
	}
 	return (*this);
}

//Deconstructor
Config::~Config() {
	std::vector<InfoServer*>::iterator	servIt;

	for(servIt = _servlist.begin(); servIt != _servlist.end(); servIt++)
		delete (*servIt);
}

//Constructor that takes a configFile as argument
Config::Config( const std::string& configFile) {
	parseConfigFile(configFile);
	//Handle return from parseConfigFile
}

void	Config::setServerList( const std::vector<InfoServer*> servlist ) {
	_servlist = servlist;
}

std::vector<InfoServer*>	Config::getServList( void ) const {
	return (_servlist);
}

int	Config::getServCount( void ) const {
	return (_servcount);
}


// InfoServer*	Config::matchFD( int fd ) {

// 	std::vector<InfoServer*>::iterator					servIt;
// 	std::vector<InfoServer*>							server;

// 	server = (*this).getServList();
// 	for(servIt = server.begin(); servIt != server.end(); servIt++)
// 	{
// 		if ((*servIt)->getFD() == fd)
// 			return (*servIt);
// 	}
// 	return NULL;
// }

bool	Config::ft_validServer( void )
{
	std::vector<InfoServer*>::iterator					servIt;
	std::vector<InfoServer*>							server;
	std::set<std::string>								s_port;
	std::pair<std::set<std::string>::iterator, bool>	inserted;
	std::string											name_port;

	server = (*this).getServList();
	if (server.empty())
	{
		std::cout << "Error! No server was properly parsed!" << std::endl;
		return false;
	}
	for(servIt = server.begin(); servIt != server.end(); servIt++)
	{
		name_port = (*servIt)->getIP() + (*servIt)->getPort();
		inserted = s_port.insert(name_port);
		if (!inserted.second)
		{
			std::cout << "Error, two servers have the same port!" << std::endl;
			return false;
		}
	}
	if ((int) s_port.size() != _servcount)
	{
		std::cout << "Error, not all servers were parsed!" << std::endl;
		return false;
	}
	return true;
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
	methods.insert(leftover);
	return methods;
}

bool	Config::parseLocation(std::istream &conf, InfoServer *server, const std::string location)
{
	Route		route;
	std::string	line;
	std::string	key;
	std::string	value;

	route.uri = location;
	route.internal = false;
	while (std::getline(conf, line))
	{
		//Trim lines and get rid of empty ones
		if (!trimLine(line))
			continue;

		if (line == "}") //end of block
		{
			if (route.uri == "/old-page") //This probably will be changed
			{
				server->setRoutes(route.uri, route);
				return true;
			}
			if (route.path.empty())
			{
				std::map<std::string, std::string> settingMap;
				settingMap = server->getSetting();
				if (route.internal)
					route.path =("." + settingMap["error_path"] + route.uri);
				else
				route.path =("." + settingMap["root"] + route.uri);
			}
			if (!route.uri.empty() && !route.path.empty()) //Add a uri != path?
			{
				if (location == "/cgi-bin")
					server->setCGI(route);
				server->setRoutes(route.uri, route);
				return true;
			}
			std::cout << "Error is in " << location << std::endl;
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
					route.path =("." + value + route.uri);
				else if (key == "allow")
					route.methods = parseMethods(value);
				else
					route.locSettings[key] = value;
			}
			else if (line == "internal;")
				route.internal = true;
		}
	}
	return (true);
}

//Parses a block of code that starts with "server"
bool	Config::parseServer(std::istream &conf)
{
	std::string	line;
	std::string	key;
	std::string	value;
	bool		location_ok = false;
	InfoServer	*server = new InfoServer;

	while (std::getline(conf, line))
	{
		if (!trimLine(line))
			continue;

		if (line == "}") //When we get to the end of the block, we exit
		{
			if(!server->getPort().empty() && !server->getIP().empty() && !server->getRoot().empty() && server->isIPValid(server->getIP())) //Checks for minimal info, can be made an external checker function
			{
				_servlist.push_back(server);
				return true;
			}
			delete server;
			return false; //Check if memory is lost and I need to add a delete here
		}
		else if (line.find("location ") != std::string::npos) //If we find a location block, we need to parse it
		{
			line = line.substr(line.find("location ") + 9); //Remove the "location " bit of the line
			size_t n = line.find(" ");
			std::string	location = line.substr(0, n);
			//Remove last slash
			if (location != "/" && location[location.size() - 1] == '/')
				location = location.substr(0, location.size() - 1);
			location_ok = parseLocation(conf, server, location);
			if (location_ok == false)
				std::cerr << "Error in location block!" << std::endl;
			location_ok = false;
		}
		else //It's a line that we need to split into key and value
		{
			size_t pos = line.find('\t'); //We look for the tab(s) that split the key and value
			key = line.substr(0, pos);
			value = line.substr(pos + 1, line.length() - key.length()); //To avoid the ; in the end
			//Trim again
			key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t") + 1);
			value = value.substr(value.find_first_not_of(" \t"), value.find_last_not_of(" \t") - value.find_first_not_of(" \t"));

			server->setSetting(key, value);
			if (key == "port") //These are special cases that we may want to have outside of the map, keep for now, can be expanded
			{
				if(atoi(value.c_str()) > 0 && atoi(value.c_str()) < 65535 && (value.find_first_not_of("0123456789") == std::string::npos))
					server->setPort(value); //Saved as string, change to int?
				else
					std::cerr << "Incorrect port value: " << value << std::endl;
			}
			if (key == "server_name") //host?
			{
				if (server->isIPValid(value))
					server->setIP(value); //Saved as string
				else
					std::cerr << "Incorrect IP: " << value << std::endl;
			}
			if (key == "root")
				server->setRoot(value);
			if (key == "index")
				server->setIndex(value);
			//Expand if we need more variables
		}
	}
	std::cout << "This should not have happened (parseServer)" << std::endl;
	delete server;
	return false; //We should never get here, so if we do, something is messed up somewhere
}


// Removes spaces and tabs at the beginning and end of a line, checks if the line is empty or is a comment. Returns false if the line is not valid (and therefore needs to be skipped) and true if the line is trimmed.
bool	Config::trimLine(std::string& line)
{
	//Trim lines and get rid of empty ones
	size_t	start = line.find_first_not_of(" \t");
	size_t	end = line.find_last_not_of(" \t");
	if (start == std::string::npos || end == std::string::npos)
		return (false);
	line = line.substr(start, end - start + 1);
	if (line.empty() || line[0] == '#')
		return (false);
	return (true);
}

//Parses the config file and populates all the attributes of the Config class
bool	Config::parseConfigFile(const std::string &configFile)
{
	std::vector<InfoServer*>	server_list;
	std::string			line;
	std::ifstream 		conf(configFile.c_str());
	bool				started_server = false; //This is the flag for whether there are errors that need solving. Currently if an error is returned, it ignores the server block and moves on

	if (!conf) //Checks if the file is accessible
	{
		std::cerr << "Error: Unable to open file " << configFile << std::endl;
		return false;
	}

	//reads line by line, extracts all parameters and location blocks
	_servcount = 0;
	while (std::getline(conf, line))
	{
		if (!trimLine(line))
			continue;

		if (line.find("server ") != std::string::npos)
		{
			_servcount++;
			started_server = parseServer(conf);
			if (started_server == false)
				std::cout << "Something went wrong in parseServer, skipped one server" <<std::endl;
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
	//What if server is empty for some reason? Add check to exit
	return true;
}
