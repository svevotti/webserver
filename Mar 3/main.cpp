#include "WebServer.hpp"
#include "InfoServer.hpp"
#include <iostream>

int main()
{
    // Create and configure InfoServer
    InfoServer info;
    info.setServerNumber(1);         // Set number of servers to 1
    info.setArrayPorts("8080");      // Add port 8080
    info.setServerRootPath("./");    // Optional: set root path
    info.setServerDocumentRoot("./www"); // Optional: set document root

    try 
    {
        Webserver server(info);
        std::cout << "Starting web server on port 8080..." << std::endl;
        server.startServer(info);
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}