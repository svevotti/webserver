#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include "CGI.hpp"
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <sys/poll.h>

// Forward declarations
class Server;         // Teammate’s Server class
class ClientRequest;
class ServerResponse;

class Server {

    private:

        // TRACKERS
        struct CGITracker {
            CGI* cgi;
            int client_fd;
            std::string input_data; 
        };

        struct ClientTracker {
            int fd;
            time_t start_time;
        };

        struct ResponseTracker {
            int fd;
            std::string response;
            std::string method;
        };

        std::map<int, CGITracker> cgi_map;              // Map pipe FDs to CGITracker instances
        std::vector<ClientTracker> client_trackers;     // Track client FDs with timeouts
        std::vector<struct pollfd> poll_sets;           // All FDs for poll()
        std::vector<ResponseTracker> response_queue;    // Queue responses to send via POLLOUT

        // Helper functions for startServer
        void setupServerSockets(Server server, int* arraySockets);
        bool runPollLoop(Server server, int* arraySockets);
        void dispatchEvents(Server server, int* arraySockets, int returnPoll);
        bool isClientTimedOut(int fd) const;
        void initializeServerSockets(Server server, int* arraySockets);
        void addServerSocketToPoll(int fd);
        void handleServerSocketEvent(int fd);
        void handleClientSocketEvent(int fd, Server server, std::vector<struct pollfd>::iterator& it);
        void handleCGIInput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void handleCGIOutput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void cleanupCGI(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void handleSendResponse(std::vector<struct pollfd>::iterator& it);

        // Helper functions for serverParsingAndResponse
        bool parseRequest(const char* str, int size, ClientRequest& request, 
                        std::map<std::string, std::string>& httpRequestLine);
        void handleCGIRequest(const ClientRequest& request, const std::string& uri, int fd, const Server& server);
        void handleNonCGIRequest(const std::string& method, const std::string& uri, Server server, 
                                const ClientRequest& request, const char* str, int size, int fd);
        void queueResponse(int fd, const std::string& response, const std::string& method = "");
        int readData(int fd, std::string& buffer, int& totBytes);


    public:

        void startServer(Server server);
        void serverParsingAndResponse(const char *str, Server server, int fd, int size);

        Server() {}
        ~Server() {}
};

#endif