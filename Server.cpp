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

Server::Server(int port, std::string ip, std::string root, std::string index) : _port(port), _ip(ip), _root(root), _index(index) {}

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
		if (ip[i] == ".")
		{
			if (j <= 0 || j > 2)
				return false;
			num.substr(i - j, j);
			if (atoi(num) < 0 || atoi(num) > 255)
				return false;
			j = 0;
		}
		else
			j++;
	}
	if (ndots != 3)
		return false;
	return true;
}
