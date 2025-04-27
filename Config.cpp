#include "Config.hpp"
#include "Logger.hpp"

//Constructor that takes a configFile as argument
Config::Config( const std::string& configFile) {
	parseConfigFile(configFile);
}

// Copy constructor
Config::Config(const Config& copy) : _servcount(copy._servcount) {
	std::vector<InfoServer*> server = copy.getServList();
	for (std::vector<InfoServer*>::const_iterator servIt = server.begin(); servIt != server.end(); ++servIt) {
		InfoServer* new_server = new InfoServer(**servIt);
		_servlist.push_back(new_server);
	}
}

//Equal operator
Config&	Config::operator=( const Config& copy) {
	if (this != &copy)
	{
		for (std::vector<InfoServer*>::iterator servIt = _servlist.begin(); servIt != _servlist.end(); ++servIt)
			delete *servIt;
		_servlist.clear();
		_servcount = copy.getServCount();
		std::vector<InfoServer*> server = copy.getServList();
		for (std::vector<InfoServer*>::iterator servIt = server.begin(); servIt != server.end(); servIt++)
		{
			InfoServer *new_server= new InfoServer(**servIt);
			_servlist.push_back(new_server);
		}
	}
 	return (*this);
}

//Deconstructor
Config::~Config() {
	for(std::vector<InfoServer*>::iterator servIt = _servlist.begin(); servIt != _servlist.end(); servIt++)
		delete (*servIt);
}

//Setters and getters
void	Config::setServerList( const std::vector<InfoServer*> servlist ) {
	_servlist = servlist;
}

std::vector<InfoServer*>	Config::getServList( void ) const {
	return (_servlist);
}

int	Config::getServCount( void ) const {
	return (_servcount);
}

bool	Config::ft_validServer( void )
{
	std::vector<InfoServer*>							server;
	std::set<std::string>								s_port;
	std::pair<std::set<std::string>::iterator, bool>	inserted;

	server = (*this).getServList();
	if (server.empty())
	{
		Logger::error("Error! No server was properly parsed!");
		return false;
	}
	for(std::vector<InfoServer*>::iterator servIt = server.begin(); servIt != server.end(); servIt++)
	{
		std::string name_port = (*servIt)->getIP() + (*servIt)->getPort();
		inserted = s_port.insert(name_port);
		if (!inserted.second)
		{
			Logger::error("Error, two servers have the same port and IP combination!");
			return false;
		}
	}
	if ((int) s_port.size() != _servcount)
	{
		Logger::error("Error, not all servers were parsed!");
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
			Logger::error("Error parsing locationg block: " + location);
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
			Logger::error("Error parsing server");
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
				Logger::error("Error in location block!");
			location_ok = false;
		}
		else //It's a line that we need to split into key and value
		{
			size_t pos = line.find('\t'); //We look for the tab(s) that split the key and value
			if ( pos != std::string::npos)
			{
				key = line.substr(0, pos);
				value = line.substr(pos + 1); //To avoid the ; in the end
				//Trim again
				key = key.substr(key.find_first_not_of(" \t"), key.find_last_not_of(" \t") + 1);
				size_t semiPos = value.find(';');
				if (semiPos != std::string::npos)
					value = value.substr(0, semiPos);
				size_t start = value.find_first_not_of(" \t");
				size_t end = value.find_last_not_of(" \t");
				if (start != std::string::npos && end != std::string::npos && end >= start)
					value = value.substr(start, end - start + 1);
				else
					value = "";
				if (key.empty() || value.empty())
				{
					Logger::warn("Invalid key-value pair: key='" + key + "', value='" + value + "'");
					continue;
				}
				std::map<std::string, std::string> settings = server->getSetting();
				if (settings.find(key) != settings.end())
					Logger::warn("Duplicate key '" + key + "' in server block, overwriting with value '" + value + "'");

				server->setSetting(key, value);
				if (key == "port") //These are special cases that we may want to have outside of the map, keep for now, can be expanded
				{
					if(atoi(value.c_str()) > 0 && atoi(value.c_str()) < 65535 && (value.find_first_not_of("0123456789") == std::string::npos))
						server->setPort(value); //Saved as string, change to int?
					else
						Logger::error("Incorrect port value: " + value);
				}
				if (key == "server_name") //host?
				{
					if (server->isIPValid(value))
						server->setIP(value); //Saved as string
					else
						Logger::error("Incorrect IP: " + value);
				}
				if (key == "root")
					server->setRoot(value);
				if (key == "index")
					server->setIndex(value);
				//Expand if we need more variables
			}
			else
				Logger::warn("Invalid config line (missing tab): " + line);
		}
	}
	Logger::error("This should not have happened (parseServer)");
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
	size_t commentPos = line.find('#');
	if (commentPos != std::string::npos)
		line = line.substr(0, commentPos);
	// Trim again after removing comment
	start = line.find_first_not_of(" \t");
	end = line.find_last_not_of(" \t");
	if (start != std::string::npos && end != std::string::npos)
		line = line.substr(start, end - start + 1);
	return !line.empty();
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
		Logger::error("Error: Unable to open file " + configFile);
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
				Logger::warn("Something went wrong in parseServer, skipped one server");
		}
	}
	//For debugging - remove?
	Logger::debug("Config file parsed correctly");
	return true;
}
