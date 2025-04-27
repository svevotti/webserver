#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <vector>
#include <unistd.h>
#include <ctime>
#include "CGI.hpp"
#include "ClientHandler.hpp"
#include "HttpResponse.hpp"


struct CGITracker {
    CGI* cgi;              // Pointer to the CGI instance
    int pipeFd;            // Current pipe FD being polled (in_pipe, out_pipe, or err_pipe)
    int clientFd;          // Original client FD
    const InfoServer* serverConfig; // Server-specific configuration
    std::string response;  // CGI response to be sent by Webserver
    bool isActive;
    bool firstResponseSent;
    std::string bufferedOutput; // Buffer for non-chunked responses
    bool headersParsed;         // Track if headers are parsed
    bool isChunked;             // Track if chunking is active
    std::string contentType;    // Persistent storage for parsed Content-Type 
    int statusCode; // Persistent status code
    std::map<std::string, std::string> cgi_headers; // CGI headers

    // Default constructor
    CGITracker()
    : cgi(NULL), pipeFd(-1), clientFd(-1), serverConfig(NULL), response(""), 
      isActive(false), firstResponseSent(false), bufferedOutput(""), 
      headersParsed(false), isChunked(false), contentType("text/plain"), statusCode(200) {} // Initialize contentType

    // Parameterized constructor
    CGITracker(CGI* c, int pFd, int cFd, InfoServer* config)
    : cgi(c), pipeFd(pFd), clientFd(cFd), serverConfig(config), response(""), 
      isActive(true), firstResponseSent(false), bufferedOutput(""), 
      headersParsed(false), isChunked(false), contentType("text/plain"), statusCode(200) {} // Initialize contentType
};

class CGIHandler {

    public:
        CGIHandler();
        ~CGIHandler();
        CGIHandler(const CGIHandler& other);

        void startCGI(ClientHandler& client, const InfoServer& serverConfig, std::vector<struct pollfd>& poll_sets);
        
        // Handlers
        void handleCGIOutput(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
        void handleCGIError(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
        void handleCGIWrite(int fd, std::vector<struct pollfd>& poll_sets);
        void handleCGIHangup(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients);
        
        // Helpers
        std::vector<CGITracker>::iterator findCGI(int fd);
        std::vector<CGITracker>& getCGIQueue() { return _cgiQueue; }
        void setClientToPollout(int fd, std::vector<struct pollfd>& poll_sets);
        void addPipeToPoll(int fd, short events, std::vector<struct pollfd>& poll_sets);
        void checkCGITimeout(std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, std::time_t currentTime); // New

        // Moved from private to public for Webserver access - to be rechecked 
        void removeCGI(std::vector<CGITracker>::iterator it);
        void initializeFDsToClose(std::vector<CGITracker>::iterator cgiIt, int fdsToClose[3]);
        void closeCGIFDs(int fdsToClose[3], std::vector<CGITracker>::iterator cgiIt);
        void removeCGIFDsFromPollSets(std::vector<struct pollfd>& poll_sets, int fdsToClose[3]);
        std::string generateServerTimeoutResponse(std::vector<CGITracker>::iterator cgiIt);
        void killCGIForClient(int clientFd, std::vector<struct pollfd>& poll_sets);
        std::string generateServerErrorResponse(std::vector<CGITracker>::iterator cgiIt, int statusCode, const std::string& statusText);

    private:

        std::vector<CGITracker> _cgiQueue;

        // Generic helpers
        bool isUploadScript(const std::string& path) const;
        bool isLoopScript(const std::string& path) const;
        bool isFortuneScript(const std::string& path) const;
        std::string formatChunkSize(size_t size) const;
        std::string formatChunkedData(const std::string& content, bool isFinal) const;
        
        // Helpers of HandleCGIOutput
        std::vector<struct pollfd>::iterator findPollIterator(int fd, std::vector<struct pollfd>& poll_sets); // Stays as iterator return
        void handlePositiveBytes(int fd, char* buffer, ssize_t bytes, std::vector<CGITracker>::iterator cgiIt);
        void handleEOF(std::vector<CGITracker>::iterator cgiIt, std::vector<struct pollfd>& poll_sets, 
            std::vector<ClientHandler>& clients, int fdsToClose[3]);
        std::string generateCGIResponse(std::vector<CGITracker>::iterator cgiIt, const std::string& newData, bool final = false); 
        void setClientResponse(std::vector<CGITracker>::iterator cgiIt,
            std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, const std::string& cgiResponse);

        // Helpers for generateCGIResponse
        void handleTimeout(std::vector<CGITracker>::iterator cgiIt, bool isUploadPy, bool isLoopPy);
        std::string handleLoopPyResponse(std::vector<CGITracker>::iterator cgiIt, const std::string& newData, bool final);
        void parseHeaders(std::vector<CGITracker>::iterator cgiIt, const std::string& newData);
        void parseStatusHeader(std::vector<CGITracker>::iterator cgiIt, const std::string& value);
        void setErrorState(std::vector<CGITracker>::iterator cgiIt, bool isUploadPy, bool isLoopPy);
        std::string handleFortuneResponse(std::vector<CGITracker>::iterator cgiIt, bool isComplete);
        void correctStatusLine(std::vector<CGITracker>::iterator cgiIt, std::string& fullResponse);
        std::string handleUploadResponse(std::vector<CGITracker>::iterator cgiIt, bool isComplete);
        std::string handleGenericResponse(std::vector<CGITracker>::iterator cgiIt, bool isComplete, bool final);
        void determineErrorResponse(const std::string& cgi_path, int statusCode, 
          std::string& errorBody, std::string& contentType);
        HttpResponse createStandardResponse(int statusCode, const std::string& body, const std::string& contentType = "text/plain", 
            bool closeConnection = true);

};

#endif // CGI_HANDLER_HPP