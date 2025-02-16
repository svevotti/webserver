#include "InfoServer.hpp"


void InfoServer::setServerRootPath(std::string path)
{
    serverRootPath = path;
}

void InfoServer::setServerDocumentRoot(std::string path)
{
    serverDocumentRoot = path;
}

std::string InfoServer::getServerDocumentRoot(void)
{
   return serverDocumentRoot;
}

std::string InfoServer::getServerRootPath(void)
{
    return serverRootPath;
}

void InfoServer::setArrayPorts(std::string portNumber)
{
    arrayPorts.push_back(portNumber);
}

std::vector<std::string> InfoServer::getArrayPorts(void)
{
    return arrayPorts;
}
