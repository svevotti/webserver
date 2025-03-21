#include "InfoServer.hpp"

//constructor and destructor
InfoServer::InfoServer()
{
    this->serverRootPath = "/home/smazzari/repos/Github/Circle5/webserver/wwww";
    this->serverDocumentRoot = "/home/smazzari/repos/Github/Circle5/webserver/www/static";
    this->serverUploadRoot = "/home/smazzari/repos/Github/Circle5/webserver/www/upload";
    this->serverErrorPath = "/home/smazzari/repos/Github/Circle5/webserver/www/error";
	// this->serverRootPath = "/Users/sveva/repos/Circle5/webserver/server_root";
	// this->serverDocumentRoot = "/Users/sveva/repos/Circle5/webserver/server_root/public_html";
    this->arrayPorts.push_back("8080");
    this->arrayPorts.push_back("9090");
    //TODO: add logger in configuration file
}

InfoServer::InfoServer(InfoServer const &other)
{
    this->serverRootPath = other.serverRootPath;
    this->serverDocumentRoot = other.serverDocumentRoot;
	this->serverRootPath = other.serverRootPath;
    this->serverUploadRoot = other.serverUploadRoot;
	this->serverDocumentRoot = other.serverDocumentRoot;
    this->serverErrorPath = other.serverErrorPath;
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
        this->serverUploadRoot = other.serverUploadRoot;
        this->serverErrorPath = other.serverErrorPath;
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

std::string InfoServer::getServerUploadRoot(void) const
{
    return serverUploadRoot;
}

std::string InfoServer::getServerErrorRoot(void) const
{
    return serverErrorPath;
}

std::vector<std::string> InfoServer::getArrayPorts(void) const
{
    return arrayPorts;
}

