#ifndef WEBSERVER_H // Include guard to prevent multiple inclusions
#define WEBSERVER_H

#include <stdio.h>          // For C-style I/O (e.g., FILE operations)
#include <stdlib.h>         // For standard C functions (e.g., malloc, free)
#include <vector>           // For std::vector (used for queues and poll sets)
#include <map>              // For std::map (used in request parsing)
#include <iostream>         // For std::cout/std::cerr (debugging output)
#include <fstream>          // For file operations (e.g., reading static pages)
#include <string.h>         // For C-string operations (e.g., strlen)
#include <sys/socket.h>     // For socket operations (e.g., socket, accept)
#include <netinet/in.h>     // For internet address structs (e.g., sockaddr_in)
#include <arpa/inet.h>      // For IP address conversion (e.g., inet_ntoa)
#include <sstream>          // For std::ostringstream (logging)
#include <stdexcept>        // For std::runtime_error (exception base)
#include <dirent.h>         // For directory operations (e.g., opendir)
#include <cstring>          // For C-string manipulation (e.g., strcmp)
#include "ClientRequest.hpp" // For parsing client HTTP requests
#include "ServerResponse.hpp"// For generating server responses
#include "InfoServer.hpp"    // For server configuration details
#include "ServerSockets.hpp" // For managing server socket creation
#include "Logger.hpp"        // For logging debug/info/error messages
#include "CGI.hpp"           // For CGI script execution

// Moved definitions here for better visibility (more standard practice)
//#define BUFFER 1024 // Buffer size for reading data (defined in implementation instead)
//#define MAX 100     // Max number of server sockets (defined in implementation instead)

// Struct to track client state
typedef struct ClientTracker {
    int fd;                  // Client socket file descriptor
    ClientRequest request;   // Parsed HTTP request from client
    std::string response;    // Response to send back to client
} ClientTracker;

// Struct to track CGI process state
typedef struct CGITracker {
    CGI* cgi;                // Pointer to CGI object managing the script
    int fd;                  // Current pipe FD (e.g., in_pipe[1] for input, out_pipe[0] for output)
    int clientFd;            // Original client FD associated with this CGI
    std::string input_data;  // Input data sent to CGI (for logging/debugging)
} CGITracker;

// Webserver class: Manages server sockets, clients, and CGI processes
class Webserver {
public:
    // Constructor: Initializes server with configuration
    Webserver(InfoServer&);
    
    // Destructor: Cleans up resources
    ~Webserver();

    // Core server functionality
    void startServer(); // Starts the main poll loop to handle events
    
    // Socket management
    void addServerSocketsToPoll(std::vector<int>); // Adds server sockets to poll set
    int fdIsServerSocket(int);                     // Checks if an FD is a server socket

    // Event dispatching and handling
    void dispatchEvents();                          // Dispatches poll events to handlers
    void handlePollInEvent(int fd);                // Handles POLLIN (data to read)
    void handlePollOutEvent(int fd);               // Handles POLLOUT (ready to write)
    void handlePollErrorOrHangup(int fd);          // Handles POLLERR or POLLHUP (errors/hangups)
    void handleCgiOutputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt); // Handles CGI output pipe errors
    void handleCgiInputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt);  // Handles CGI input pipe errors
    void handleClientError(int fd);                // Handles client disconnection or errors

    // Client management
    void createNewClient(int);                     // Accepts a new client connection
    int readData(int, std::string&, int&);         // Reads data from a socket into a buffer
    void handleReadEvents(int);                    // Processes read events from clients
    void handleClientDisconnect(int fd, int bytesRecv); // Handles client disconnection during read
    void processRequest(int fd, int contentLength);     // Processes a complete client request

    // CGI management
    void setupCGI(int fd, std::vector<struct ClientTracker>::iterator& clientIt); // Sets up CGI for a request
    void handleCgiOutput(int fd, std::vector<CGITracker>::iterator& cgiIt);       // Reads CGI output
    void handleWritingEvents(int);                 // Handles writing events (client or CGI)
    void writeCGIInput(int fd, std::vector<CGITracker>::iterator& cgiIt);         // Writes input to CGI stdin
    void sendClientResponse(int fd, std::vector<struct ClientTracker>::iterator& iterClient); // Sends response to client

    // Request and response utilities
    ClientRequest ParsingRequest(std::string, int); // Parses raw HTTP request
    std::string prepareResponse(ClientRequest);     // Prepares response for non-CGI requests

    // Helper methods
    void closeSockets();                            // Closes all sockets in poll_sets
    int searchPage(std::string path);               // Checks if a file exists
    int isCGI(std::string);                         // Determines if a URI requires CGI
    std::vector<struct ClientTracker>::iterator retrieveClient(int fd); // Finds client by FD
    std::vector<struct CGITracker>::iterator retrieveCGI(int fd);       // Finds CGI tracker by FD

private:
    // Private members
    InfoServer *_serverInfo;           // Pointer to server configuration (consider renaming to avoid confusion with class name)
    std::vector<struct pollfd> poll_sets; // List of FDs to poll (server sockets, clients, CGI pipes)
    std::vector<struct pollfd>::iterator it; // Iterator for traversing poll_sets
    std::vector<struct pollfd>::iterator end; // End iterator for poll_sets
    int totBytes;                      // Total bytes read from a client
    std::string full_buffer;           // Buffer for accumulating client request data
    std::vector<int> serverFds;        // List of server socket FDs
    std::vector<struct ClientTracker> clientsQueue; // Queue of active clients
    std::vector<struct CGITracker> cgiQueue;        // Queue of active CGI processes
};

#endif // WEBSERVER_H