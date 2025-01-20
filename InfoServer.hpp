#ifndef INFO_SERVER_H
#define INFO_SERVER_H

#include <vector>
#include <iostream>
#include <string>

class InfoServer
{
    public:
        void setArrayPorts(std::string);
        std::vector<std::string> getArrayPorts();
        void setConfigFilePath(std::string);
        std::string getConfigFilePath();
    private:
        std::vector<std::string> arrayPorts;
        std::string ConfigFilePath;
};








#endif