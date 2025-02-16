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
        void setServerRootPath(std::string);
        std::string getServerRootPath();
        void setServerDocumentRoot(std::string);
        std::string getServerDocumentRoot();
    private:
        std::vector<std::string> arrayPorts;
        std::string serverRootPath;
        std::string serverDocumentRoot;
};








#endif