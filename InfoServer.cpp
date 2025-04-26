#include "InfoServer.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

//default constructor (only for orthodox form)
// Simona / debugging leaks - add initialization to -1
InfoServer::InfoServer() : _fd(-1) {}

// Copy constructor
InfoServer::InfoServer( const InfoServer& copy) {
	*this = copy;
}

// Assignment operator // siMONA CHANGES CHASING LEAKS
InfoServer&	InfoServer::operator=( const InfoServer& copy) {
	if (this != &copy)
	{
		_port = copy.getPort();
		_ip = copy.getIP();
		_root = copy.getRoot();
		_index = copy.getIndex();
		_cgi = copy.getCGI();
		_settings = copy.getSetting();
		_routes = copy.getRoute();
		_fd = -1; // Simona / debugging leaks - add initialization to -1
	}
	return (*this);
}

//Deconstructor
InfoServer::~InfoServer() {
}
// Constructor with parameters 
InfoServer::InfoServer(std::string port, std::string ip, std::string root, std::string index) 
    : _port(port), _ip(ip), _root(root), _index(index), _fd(-1) { // CHANGED: Added _fd(-1) initialization
}

// Setters 
void	InfoServer::setPort( std::string port ) {
	this->_port = port;
}

void	InfoServer::setIP( std::string ip ) {
	if (ip == "localhost")
		this->_ip = "127.0.0.1";
	else
		this->_ip = ip;
}

void	InfoServer::setRoot( std::string root ) {
	this->_root = root;
}

void	InfoServer::setIndex( std::string index ) {
	this->_index = index;
}

void	InfoServer::setRoutes( const std::string &uri, const Route &route){
	this->_routes[uri] = route;
}

void	InfoServer::setCGI( const Route &route){
	this->_cgi = route;
}

void	InfoServer::setSetting( const std::string key, const std::string &value) {
	this->_settings[key] = value;
}

void	InfoServer::setFD( int fd ) {
	this->_fd = fd;
}

// Getters
std::string	InfoServer::getPort( void ) const {
	return (_port);
}

std::string	InfoServer::getIP ( void ) const {
	return (_ip);
}

std::string	InfoServer::getRoot ( void ) const {
	return (_root);
}

std::string	InfoServer::getIndex ( void ) const {
	return (_index);
}

std::map<std::string, std::string>	InfoServer::getSetting ( void ) const {
	return (_settings);
}

std::map<std::string, Route>	InfoServer::getRoute ( void ) const {
	return (_routes);
}

Route	InfoServer::getCGI ( void ) const {
	return (_cgi);
}

int	InfoServer::getFD ( void ) const {
	return (_fd);
}

// Simona: Added getter for cgi_processing_timeout
// Explanation: this function retrieves the CGI processing timeout from the settings map.
// If not found, it returns a default value of 15 seconds.
double InfoServer::getCGIProcessingTimeout(void) const 
{
    std::map<std::string, std::string>::const_iterator it = _settings.find("cgi_processing_timeout");
    if (it != _settings.end()) 
	{
		double value = atof(it->second.c_str());
		return value;
	}	
	Logger::debug("cgi_processing_timeout not found in settings, using default: 45.0");
    return 15.0; // Default to 15 seconds if not specified
}

// IP validation
bool	InfoServer::isIPValid( std::string ip ) 
{
	int	ndots = 0;
	if (ip == "localhost")
		return true;
	for (int i = 0; ip[i]; i++)
	{
		if (ip[i] == '.')
			ndots++;
		else if (isdigit(ip[i]))
			continue;
		else
			return false;
	}
	int	j = 0;
	std::string	num;
	for (int i = 0; ip[i]; i++)
	{
		if (ip[i] == '.' || i + 1 == (int) ip.length())
		{
			if ((i - j) > 3)
				return false;
			if (i + 1 == (int) ip.length())
				num = ip.substr(j, i - j + 1);
			else
				num = ip.substr(j, i - j);
			if (atoi(num.c_str()) < 0 || atoi(num.c_str()) > 255)
				return false;
			j = i + 1;
		}
	}
	if (ndots != 3)
		return false;
	return true;
}

