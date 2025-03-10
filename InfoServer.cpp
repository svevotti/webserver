#include "InfoServer.hpp"

//constructor and destructor
InfoServer::InfoServer()
{
    // this->serverRootPath = "/home/smazzari/repos/Github/Circle5/webserver/server_root";
    // this->serverDocumentRoot = "/home/smazzari/repos/Github/Circle5/webserver/server_root/public_html";
	this->serverRootPath = "/Users/sveva/repos/Circle5/webserver/server_root";
	this->serverDocumentRoot = "/Users/sveva/repos/Circle5/webserver/server_root/public_html";
    this->arrayPorts.push_back("8080");
    this->arrayPorts.push_back("9090");
    //TODO: add logger in configuration file
}

InfoServer::InfoServer(InfoServer const &other)
{
    this->serverRootPath = other.serverRootPath;
    this->serverDocumentRoot = other.serverDocumentRoot;
	this->serverRootPath = other.serverRootPath;
	this->serverDocumentRoot = other.serverDocumentRoot;
    this->arrayPorts = other.arrayPorts;
}

InfoServer &InfoServer::operator=(InfoServer const &other)
{
    if (this != &other)
    {
        this->serverRootPath = other.serverRootPath;
        this->serverDocumentRoot = other.serverDocumentRoot;
        this->serverRootPath = other.serverRootPath;
        this->serverDocumentRoot = other.serverDocumentRoot;
        this->arrayPorts = other.arrayPorts;
    }
    return *this;
}
//getters and setters
std::string InfoServer::getServerDocumentRoot(void) const
{
   return serverDocumentRoot;
}

std::string InfoServer::getServerRootPath(void) const
{
    return serverRootPath;
}

std::vector<std::string> InfoServer::getArrayPorts(void) const
{
    return arrayPorts;
}

