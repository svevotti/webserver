#ifndef INFO_SERVER_H
#define INFO_SERVER_H

#include <vector>
#include <iostream>
#include <string>

class InfoServer {

    public:
    
        void setArrayPorts(std::string);
        std::vector<std::string> getArrayPorts() const; // Added const
        void setServerRootPath(std::string);
        std::string getServerRootPath() const;         // Added const
        void setServerDocumentRoot(std::string);
        std::string getServerDocumentRoot() const;     // Added const
        int getServerNumber() const;
        void setServerNumber(int);

    private:
        std::vector<std::string> arrayPorts;
        std::string serverRootPath;
        std::string serverDocumentRoot;
        int serverNumbers;
};

#endif