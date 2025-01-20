#include "InfoServer.hpp"


void InfoServer::setConfigFilePath(std::string path)
{
    ConfigFilePath = path;
}
std::string InfoServer::getConfigFilePath(void)
{
    return ConfigFilePath;
}

void InfoServer::setArrayPorts(std::string portNumber)
{
    arrayPorts.push_back(portNumber);
}

std::vector<std::string> InfoServer::getArrayPorts(void)
{
    return arrayPorts;
}
