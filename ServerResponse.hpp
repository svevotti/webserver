#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <map>
#include <cstdio>

#include "ClientRequest.hpp"
#include "InfoServer.hpp"

#define HTML 0
#define IMAGE 1

class ServerResponse {
	public:
		ServerResponse(ClientRequest, InfoServer);
		ServerResponse	&operator=(ServerResponse const &other);

		std::map<int, std::string> getStatusCode() const;

		std::string 	responseGetMethod();
		std::string		getFileContent(std::string, std::string);
		int				getContentType(std::string);
		std::string		extractHtml(std::ifstream&);
		std::string		extractImage(std::ifstream&);

		std::string		responseDeleteMethod();

		std::string		responsePostMethod();
		std::string		handleFilesUploads();
		std::string		getFileType(std::map<std::string, std::string> headers);
		std::string		getFileName(std::map<std::string, std::string> headers);
		int				checkNameFile(std::string str, std::string path);

		std::string		generateHttpResponse(std::string length);
		std::string		generateStatusCode(int code);

		void			createMapStatusCode();
		std::string		pageNotFound(void);

	private:
		ClientRequest				request;
		InfoServer					info;
		std::map<int, std::string>	mapStatusCode;
};

#endif