#include "InfoServer.hpp"


void InfoServer::setArrayPorts(std::string portNumber)
{
    arrayPorts.push_back(portNumber);
}

std::vector<std::string> InfoServer::getArrayPorts(void)
{
    return arrayPorts;
}
