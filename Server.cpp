#include "Server.hpp"

//default constructor (only for orthodox form)
Server::Server( void ) {
}

//Copy constructor
Server::Server( const Server& copy) {
	*this = copy;
}

//Equal operator
Server&	Server::operator=( const Server& copy) {
	if (this != &copy)
	{
		return (*this);
	}

	return (*this);
}

//Deconstructor
Server::~Server() {
}

Server::Server(std::string port, std::string ip, std::string root, std::string index) : _port(port), _ip(ip), _root(root), _index(index) {}

void	Server::setPort( std::string port ) {
	this->_port = port;
}

void	Server::setIP( std::string ip ) {
	if (ip == "localhost")
		this->_ip = "127.0.0.1";
	else
		this->_ip = ip;
}
void	Server::setRoot( std::string root ) {
	this->_root = root;
}
void	Server::setIndex( std::string index ) {
	this->_index = index;
}

void	Server::setRoutes( const std::string &uri, const Route &route){
	this->_routes[uri] = route;
}

bool	Server::isIPValid( std::string ip ) {
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
