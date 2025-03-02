#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include "CGI.hpp"
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <sys/poll.h>

class InfoServer;
class ClientRequest;
class ServerResponse;

class Server {

    private:

        //Trackers
        struct CGITracker {
            CGI* cgi;               // Pointer to CGI instance
            int client_fd;          // FD of the client that triggered this CGI
            std::string input_data; // Data from client to send to CGI (empty for GET)
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

        std::map<int, CGITracker> cgi_map; // Keys: FDs from CGI pipes; 
                                        // Values: CGITracker objects linking FDs to 
                                        // CGI instances and clients
        // The point of this map:
        // tracks all active CGI processes by associating each CGI pipe FD 
        // (e.g., _in_pipe[1] for input, _out_pipe[0] for output) 
        // with its corresponding CGITracker 
        // (a CGIT Tracker has form: {cgi_ptr, 3, "data=example"}).
        // 3 is the client_fd
        // "data_example" is the POST body to send to CGI.
        // Thanks to this structure we can ask: 
        // “For FD 5, which CGI and client does it belong to?”
        // cgi_map[5] gives us the relevant CGITracker.
        // Overall: keeps CGI FDs tied to their clients, for routing responses.
        
        std::vector<ClientTracker> client_trackers;     // Track client FDs with timeouts
        std::vector<struct pollfd> poll_sets;           // All FDs for poll()
        std::vector<ResponseTracker> response_queue;    // Queue responses to send via POLLOUT

        // Helper functions for startServer
        void setupServerSockets(InfoServer info, int* arraySockets);
        bool runPollLoop(InfoServer info, int* arraySockets);
        void dispatchEvents(InfoServer info, int* arraySockets, int returnPoll);
        bool isClientTimedOut(int fd) const;
        void initializeServerSockets(InfoServer info, int* arraySockets);
        void addServerSocketToPoll(int fd);
        void handleServerSocketEvent(int fd);
        void handleClientSocketEvent(int fd, InfoServer info, std::vector<struct pollfd>::iterator& it);
        void handleCGIInput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void handleCGIOutput(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void cleanupCGI(CGITracker& tracker, std::vector<struct pollfd>::iterator& it);
        void handleSendResponse(std::vector<struct pollfd>::iterator& it);

        // Helper functions for serverParsingAndResponse
        bool parseRequest(const char* str, int size, ClientRequest& request, 
                        std::map<std::string, std::string>& httpRequestLine);
        void handleCGIRequest(const ClientRequest& request, const std::string& uri, int fd, const InfoServer& info);
        void handleNonCGIRequest(const std::string& method, const std::string& uri, InfoServer info, 
                                const ClientRequest& request, const char* str, int size, int fd);
        void queueResponse(int fd, const std::string& response, const std::string& method = "");
        int readData(int fd, std::string& buffer, int& totBytes);


    public:
        
        void startServer(InfoServer info);
        void serverParsingAndResponse(const char *str, InfoServer info, int fd, int size);

        Server() {}
        ~Server() {}
};

#endif