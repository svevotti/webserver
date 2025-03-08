#ifndef INFO_SERVER_H
#define INFO_SERVER_H

#include <vector>
#include <iostream>
#include <string>

class InfoServer {

    public:
        InfoServer();
        InfoServer(InfoServer const &other);

        InfoServer &operator=(InfoServer const &other);
        
        // Getters
        std::vector<std::string> getArrayPorts() const;
        std::string getServerRootPath() const;
        std::string getServerDocumentRoot() const;

        // Setters (New, useful to Simona for testing)
        void setArrayPorts(const std::string& ports); // New
        void setServerRootPath(const std::string& path); // New 
        void setServerDocumentRoot(const std::string& path); // New
        
    private:
        std::vector<std::string> arrayPorts;
        std::string serverRootPath;
        std::string serverDocumentRoot;
};








#endif