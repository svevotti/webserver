#include <sys/socket.h>
#include <iostream>
#include <sys/types.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerSockets.hpp"
#include "InfoServer.hpp"
#include "Webserver.hpp"
#include <signal.h>


//SVEVA's
//int main(void)
//{
//	InfoServer		info;
//	Webserver 	server(info);
//
//	server.startServer();
//	return (0);
//}

//SIMONA's
int main()
{
    // Create InfoServer with defaults
    InfoServer info;
    // Defaults from InfoServer:
    // - Ports: 8080, 9090
    // - serverRootPath: "/home/smazzari/repos/Github/Circle5/webserver/server_root"
    // - serverDocumentRoot: "/home/smazzari/repos/Github/Circle5/webserver/server_root/public_html"

    // Override defaults for testing
    info.setArrayPorts("8080");      // Use only 8080 (clears 9090)
    info.setServerRootPath("./");    // Simpler root for testing
    info.setServerDocumentRoot("./www"); // Simpler document root for testing

    try 
    {
        Webserver server(info);
        std::cout << "Starting web server on port 8080..." << std::endl;
        server.startServer();  // Uses _serverInfo internally
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}