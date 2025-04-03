#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "CGI.hpp"
#include "ClientHandler.hpp"
#include "HttpResponse.hpp"
#include <vector>

struct CGITracker {
    CGI* cgi;              // Pointer to the CGI instance
    int pipeFd;            // Current pipe FD being polled (in_pipe, out_pipe, or err_pipe)
    int clientFd;          // Original client FD
    std::string response;  // CGI response to be sent by Webserver
};

class CGIHandler {
public:
    CGIHandler();
    ~CGIHandler();

    void startCGI(ClientHandler& client, const std::string& raw_data, const InfoServer& serverConfig,
                  std::vector<struct pollfd>& poll_sets);
    
    // Handlers (updated to void)
    void handleCGIOutput(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
    void handleCGIError(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
    void handleCGIWrite(int fd, std::vector<struct pollfd>& poll_sets);
    void handleCGIHangup(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
    
    // Utils
    std::vector<CGITracker>::iterator findCGI(int fd);
    std::vector<CGITracker>& getCGIQueue() { return _cgiQueue; }

private:
    std::vector<CGITracker> _cgiQueue;
    void removeCGI(std::vector<CGITracker>::iterator it);

    // Helpers of HandleCGIOutput (updated to void where applicable)
    std::vector<struct pollfd>::iterator findPollIterator(int fd, std::vector<struct pollfd>& poll_sets); // Stays as iterator return
    void initializeFDsToClose(std::vector<CGITracker>::iterator cgiIt, int fdsToClose[3]);
    void handlePositiveBytes(int fd, char* buffer, ssize_t bytes,
        std::vector<CGITracker>::iterator cgiIt, std::vector<struct pollfd>& poll_sets,
        std::vector<ClientHandler>& clients, std::vector<struct pollfd>::iterator pollIt, int fdsToClose[3]);
    void handleEOF(int fd, std::vector<CGITracker>::iterator cgiIt,
        std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients,
        std::vector<struct pollfd>::iterator pollIt, int fdsToClose[3]);
    std::string generateCGIResponse(std::vector<CGITracker>::iterator cgiIt);
    void setClientResponse(std::vector<CGITracker>::iterator cgiIt,
        std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, const std::string& cgiResponse);
    void closeCGIFDs(int fdsToClose[3], std::vector<CGITracker>::iterator cgiIt);
    void removeCGIFDsFromPollSets(std::vector<struct pollfd>& poll_sets, int fdsToClose[3]);
};

#endif // CGI_HANDLER_HPP