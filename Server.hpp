#ifndef SERVER_H
# define SERVER_H

# include "Config.hpp"

class Server {
	private:
		Server( void );
		std::string	_port;
		std::string	_ip;
		std::string	_root;
		std::string	_index;

	public:
		Server(int port, std::string ip, std::string root, std::string index);
		Server( const Server& copy);
		Server& operator=(const Server& copy);
		~Server();

		void	setPort( std::string port );
		void	setIP( std::string ip );
		void	setRoot( std::string root );
		void	setIndex( std::string index );

		bool	isIPValid( std::string ip );
};

#endif
