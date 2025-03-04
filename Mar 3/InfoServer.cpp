#include "InfoServer.hpp"

void InfoServer::setServerRootPath(std::string path)
{
    serverRootPath = path;
}

void InfoServer::setServerDocumentRoot(std::string path)
{
    serverDocumentRoot = path;
}

std::string InfoServer::getServerDocumentRoot() const // Added const, removed (void)
{
   return serverDocumentRoot;
}

std::string InfoServer::getServerRootPath() const // Added const, removed (void)
{
    return serverRootPath;
}

void InfoServer::setArrayPorts(std::string portNumber)
{
    arrayPorts.push_back(portNumber);
}

std::vector<std::string> InfoServer::getArrayPorts() const // Added const, removed (void)
{
    return arrayPorts;
}

void InfoServer::setServerNumber(int number)
{
    this->serverNumbers = number;
}

int InfoServer::getServerNumber() const
{
    return serverNumbers;
}