#include "InfoServer.hpp"

//default constructor (only for orthodox form)
InfoServer::InfoServer( void ) {
}

//Copy constructor
InfoServer::InfoServer( const InfoServer& copy) {
	*this = copy;
}

//Equal operator
InfoServer&	InfoServer::operator=( const InfoServer& copy) {
	if (this != &copy)
	{
		return (*this);
	}

	return (*this);
}

//Deconstructor
InfoServer::~InfoServer() {
}

InfoServer::InfoServer(std::string port, std::string ip, std::string root, std::string index) : _port(port), _ip(ip), _root(root), _index(index) {}

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

bool	InfoServer::isIPValid( std::string ip ) {
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
