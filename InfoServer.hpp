#ifndef INFO_SERVER_H
#define INFO_SERVER_H

#include <vector>
#include <iostream>
#include <string>

class InfoServer
{
    public:
        InfoServer();
        InfoServer(InfoServer const &other);

        InfoServer &operator=(InfoServer const &other);
        
        std::vector<std::string> getArrayPorts() const;
        std::string getServerRootPath() const;
        std::string getServerDocumentRoot() const;
    private:
        std::vector<std::string> arrayPorts;
        std::string serverRootPath;
        std::string serverDocumentRoot;
};








#endif