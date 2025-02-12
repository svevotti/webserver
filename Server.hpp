#ifndef SERVER_H
# define SERVER_H

# include <iostream>
# include <cstdlib>
# include <map>
# include <vector>
# include <fstream>
# include <sstream>
# include <map>
# include <string>
# include <set>

struct Route
{
	std::string	uri;
	std::string	path;
	std::set<std::string> methods;
};


class Server {
	private:
		Server( void );

	public:
		std::string	_port;
		std::string	_ip;
		std::string	_root;
		std::string	_index;
		std::map<std::string, Route> _routes;
		Server(std::string port, std::string ip, std::string root, std::string index);
		Server( const Server& copy);
		Server& operator=(const Server& copy);
		~Server();

		void	setPort( std::string port );
		void	setIP( std::string ip );
		void	setRoot( std::string root );
		void	setIndex( std::string index );
		void	setRoutes( const std::string &uri, const Route &route);

		bool	isIPValid( std::string ip );
};

#endif
