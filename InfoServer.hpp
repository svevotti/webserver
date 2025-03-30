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
	std::string							uri;
	std::string							path;
	std::set<std::string>				methods;
	std::map<std::string, std::string>	locSettings;
	bool								internal;
};


class InfoServer {
	private:
		std::string							_port;
		std::string							_ip;
		std::string							_root;
		std::string							_index;
		std::map<std::string, std::string>	_settings;
		std::map<std::string, Route>		_routes;
		Route								_cgi;
		int									_fd;

	public:
		InfoServer( void );
		InfoServer(std::string port, std::string ip, std::string root, std::string index);
		InfoServer( const InfoServer& copy);
		InfoServer& operator=(const InfoServer& copy);
		~InfoServer();

		void	setPort( std::string port );
		void	setIP( std::string ip );
		void	setRoot( std::string root );
		void	setIndex( std::string index );
		void	setSetting( const std::string key, const std::string &value);
		void	setRoutes( const std::string &uri, const Route &route);
		void	setCGI( const Route &route );
		void	setFD( int fd );

		std::string							getPort( void ) const;
		std::string							getIP ( void ) const;
		std::string							getRoot ( void ) const;
		std::string							getIndex ( void ) const;
		std::map<std::string, std::string>	getSetting ( void ) const;
		std::map<std::string, Route>		getRoute ( void ) const;
		Route								getCGI( void ) const;
		int									getFD( void ) const;

		bool	isIPValid( std::string ip );
};

#endif
