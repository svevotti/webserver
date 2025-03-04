#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <deque>
#include <map>
#include <string>
#include <sys/poll.h>
#include "ServerSockets.hpp"
#include "InfoServer.hpp"
#include "ClientRequest.hpp"
#include "ServerResponse.hpp"
#include "CGI.hpp"

#define BUFFER 4096

struct ResponseTracker {
    int fd;
    std::string response;
    std::string method;
};

struct CGITracker {
    CGI* cgi;
    int fd;
    std::string input_data;
};

class Webserver {
    
    public:

        Webserver(InfoServer info);
        ~Webserver();
        void startServer(InfoServer info);

    
    private:

        std::vector<int> serverFds;
        std::vector<struct pollfd> poll_sets;
        std::deque<ResponseTracker> response_queue;
        std::map<int, CGITracker> cgi_map;
        std::string full_buffer;
        int totBytes;
        std::vector<struct pollfd>::iterator it, end;

        void addServerSocketsToPoll(const std::vector<int>& fds);
        int createNewClient(int fd);
        int readData(int fd, std::string& str, int& bytes);
        void handleReadEvents(int fd, InfoServer info); // Handles client requests (CGI or non-CGI)
        void handleCGIRequest(const ClientRequest& request, int client_fd, InfoServer info);
        void queueResponse(int fd, const std::string& response, const std::string& method);
        void handleSendResponse(std::vector<struct pollfd>::iterator& it);
        void checkCGITimeouts();
        void dispatchEvents(InfoServer info);

        // Refactored helper functions for dispatchEvents
        bool isServerSocket(int fd);
        void handleServerPollin(std::vector<struct pollfd>::iterator& it);
        bool isCGIOutputFd(int fd, CGITracker& tracker);
        void readCGIOutput(std::vector<struct pollfd>::iterator& it, CGITracker& tracker);
        void handlePollinEvents(std::vector<struct pollfd>::iterator& it, InfoServer info);
        bool isCGIInputFd(int fd, CGITracker& tracker);
        void writeCGIInput(std::vector<struct pollfd>::iterator& it, CGITracker& tracker);
        void handlePolloutEvents(std::vector<struct pollfd>::iterator& it);
        void handleCGIOutputHup(std::vector<struct pollfd>::iterator& it, CGITracker& tracker);
        void handlePollhupEvents(std::vector<struct pollfd>::iterator& it);
        void handlePollerrEvents(std::vector<struct pollfd>::iterator& it); // New function for POLLERR

};

#endif