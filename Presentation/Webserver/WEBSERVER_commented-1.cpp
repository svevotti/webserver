#include "Webserver.hpp"
#include <cstring>
#include <iostream> // Added for std::cerr

#define BUFFER 1024 // Buffer size for reading data from sockets
#define MAX 100     // Maximum number of server sockets to reserve in poll_sets

//-------------Constructor and Destructor--------------------------------------------

// Constructor: Sets up the web server with server info and initializes sockets
Webserver::Webserver(InfoServer& info)
{
    this->_serverInfo = new InfoServer(info); // Copy server configuration (ports, root, etc.)
    ServerSockets serverFds(*this->_serverInfo); // Create server sockets based on config

    // Check if any server sockets were created successfully
    if (serverFds.getServerSockets().empty())
        Logger::error("Failed creating any server socket");

    this->serverFds = serverFds.getServerSockets(); // Store server socket FDs
    this->totBytes = 0;                             // Total bytes read from clients
    this->full_buffer.clear();                      // Buffer for incoming client data
    this->poll_sets.reserve(MAX);                   // Reserve space for pollfd structs
    addServerSocketsToPoll(this->serverFds);        // Add server sockets to poll set for monitoring
}

// Destructor: Cleans up allocated resources and closes sockets
Webserver::~Webserver()
{
    delete this->_serverInfo; // Free server info memory

    // Clean up all CGI objects in the queue
    for (std::vector<CGITracker>::iterator it = cgiQueue.begin(); it != cgiQueue.end(); ++it)
        delete it->cgi;
    closeSockets(); // Close all open sockets
}

//------------------ Main functions----------------------------------------------

// Main server loop: Polls for events and dispatches them
void Webserver::startServer()
{
    int returnPoll;
    Logger::info("Entering poll loop - debug version 2023-03-07");
    while (true) // Infinite loop to keep server running
    {
        std::ostringstream oss;
        oss << poll_sets.size();
        Logger::debug(std::string("Polling ") + oss.str() + " FDs"); // Log number of FDs being polled
        // Log details of each FD in poll_sets for debugging
        for (size_t i = 0; i < poll_sets.size(); ++i)
        {
            oss.str(""); oss << "FD " << poll_sets[i].fd << " events: " << poll_sets[i].events;
            Logger::debug(std::string("Poll set entry: ") + oss.str());
        }
        // Poll with a 20-second timeout (20 * 1000 ms)
        returnPoll = poll(this->poll_sets.data(), this->poll_sets.size(), 1 * 20 * 1000);
        if (returnPoll == -1) // Poll failed
        {
            Logger::error(std::string("Failed poll: ") + std::string(strerror(errno)));
            break; // Exit loop on error
        }
        else if (returnPoll == 0) // No events occurred
            Logger::debug("Poll timeout: no events");
        else // Events detected
        {
            oss.str(""); oss << returnPoll;
            Logger::debug(std::string("Poll detected ") + oss.str() + " events");
            dispatchEvents(); // Handle the events
        }
    }
}

// Handles POLLIN events (data available to read)
void Webserver::handlePollInEvent(int fd)
{
    std::ostringstream oss;
    oss << "Handling POLLIN for FD " << fd;
    Logger::debug(oss.str());
    if (fdIsServerSocket(fd) == true) // If it’s a server socket, accept a new client
        createNewClient(fd);
    else // Otherwise, it’s a client or CGI pipe
        handleReadEvents(fd);
}

// Handles errors or hangups on CGI output pipes
void Webserver::handleCgiOutputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    std::string cgiResponse = "HTTP/1.1 "; // Base HTTP response
    std::ostringstream oss;
    if (this->it->revents & POLLERR) // Pipe error
    {
        cgiResponse += "500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        oss << fd;
        Logger::error(std::string("CGI error on FD ") + oss.str());
    }
    else // POLLHUP (pipe closed)
    {
        std::string output = cgiIt->cgi->getOutput(); // Get CGI script output
        Logger::debug(std::string("Raw CGI output: [") + output + "]");
        if (output.empty()) // No output = error
        {
            cgiResponse += "500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
            Logger::debug("CGI output empty, setting 500");
        }
        else // Process CGI output
        {
            size_t bodyStart = output.find("\r\n\r\n"); // Split headers and body
            std::string status = "200 OK"; // Default status
            if (bodyStart != std::string::npos)
            {
                std::string headers = output.substr(0, bodyStart);
                output = output.substr(bodyStart + 4); // Body starts after double CRLF
                // Check for error status in CGI headers
                if (headers.find("HTTP/1.1 500") != std::string::npos)
                    status = "500 Internal Server Error";
                else if (headers.find("HTTP/1.1 400") != std::string::npos)
                    status = "400 Bad Request";
                Logger::debug(std::string("Stripped CGI headers: [") + headers + "]");
            }
            // Trim non-printable characters from end of output
            size_t realLength = output.size();
            while (realLength > 0 && !std::isprint(static_cast<unsigned char>(output[realLength - 1])))
                --realLength;
            output.resize(realLength);
            oss.str(""); oss << output.size();
            // Build full response with headers
            cgiResponse += status + "\r\nContent-Length: " + oss.str() +
                           "\r\nConnection: keep-alive\r\n\r\n" + output;
            Logger::debug(std::string("CGI completed, output size: ") + oss.str());
            Logger::debug(std::string("CGI output content: [") + output + "]");
            oss.str(""); oss << cgiResponse.size();
            Logger::debug(std::string("Full cgiResponse size: ") + oss.str());
        }
    }

    cgiIt->cgi->closePipe(cgiIt->fd); // Close the CGI pipe
    int clientFd = cgiIt->clientFd;   // Get associated client FD
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Closing CGI pipe and storing client FD ") + oss.str());

    // Remove client from clientsQueue if it exists
    std::vector<struct ClientTracker>::iterator existing = retrieveClient(clientFd);
    if (existing != clientsQueue.end())
    {
        clientsQueue.erase(existing);
        oss.str(""); oss << clientFd;
        Logger::debug(std::string("Removed client FD ") + oss.str() + " from clientsQueue");
    }

    // Remove client FD from poll_sets
    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == clientFd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << clientFd;
            Logger::debug(std::string("Removed FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
            ++pit;
    }

    // Clean up CGI object and remove from queue
    CGI* cgiPtr = cgiIt->cgi;
    cgiQueue.erase(cgiIt);
    oss.str(""); oss << fd;
    Logger::debug(std::string("Erased CGI tracker for FD ") + oss.str());
    delete cgiPtr;
    Logger::debug("Deleted CGI object");

    // Remove CGI FD from poll_sets
    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == fd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << fd;
            Logger::debug(std::string("Erased FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
            ++pit;
    }

    // Queue the response for the client
    struct ClientTracker client = { clientFd, ClientRequest(), cgiResponse };
    clientsQueue.push_back(client);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added client FD ") + oss.str() + " to clientsQueue");

    // Add client FD back to poll_sets with POLLOUT
    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLOUT;
    poll_sets.push_back(clientPoll);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added FD ") + oss.str() + " with POLLOUT to poll_sets");
    oss.str(""); oss << poll_sets.size();
    Logger::debug(std::string("Poll sets size after queuing: ") + oss.str());
}

// Handles POLLOUT events (ready to write data)
void Webserver::handlePollOutEvent(int fd)
{
    std::ostringstream oss;
    oss << "Handling POLLOUT for FD " << fd;
    Logger::debug(oss.str());

    // Check if it’s a CGI input pipe
    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end())
    {
        if (cgiIt->fd == cgiIt->cgi->getInPipeWriteFd()) // Writing to CGI stdin
        {
            writeCGIInput(fd, cgiIt);
            return;
        }
        oss.str(""); oss << fd;
        Logger::warn(std::string("Unexpected POLLOUT on CGI output FD ") + oss.str());
        return;
    }

    // Check if it’s a client ready to receive a response
    std::vector<struct ClientTracker>::iterator clientIt = retrieveClient(fd);
    if (clientIt != clientsQueue.end())
        sendClientResponse(fd, clientIt);
    else
    {
        oss.str(""); oss << fd;
        Logger::debug(std::string("No CGI or client match for FD ") + oss.str() + ", skipping");
    }
}

// Handles client disconnection or errors
void Webserver::handleClientError(int fd)
{
    std::ostringstream oss;
    oss << fd;
    Logger::info(std::string("Client ") + oss.str() + " hung up or errored");
    close(fd); // Close the client socket
    // Remove from clientsQueue
    std::vector<ClientTracker>::iterator clientIt = retrieveClient(fd);
    if (clientIt != clientsQueue.end())
        this->clientsQueue.erase(clientIt);
    // Remove from poll_sets
    bool removed = false;
    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
    {
        if (pit->fd == fd)
        {
            this->it = poll_sets.erase(pit);
            removed = true;
            break;
        }
    }
    if (!removed)
        Logger::warn(std::string("FD ") + oss.str() + " not found in poll_sets");
}

// Handles POLLERR or POLLHUP events
void Webserver::handlePollErrorOrHangup(int fd)
{
    std::ostringstream oss;
    oss << fd;
    Logger::debug(std::string("Handling POLLERR or POLLHUP for FD ") + oss.str());

    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end()) // CGI-related FD
    {
        if (cgiIt->fd == cgiIt->cgi->getOutPipeReadFd()) // Output pipe error/hangup
            handleCgiOutputPipeError(fd, cgiIt);
        else if (cgiIt->fd == cgiIt->cgi->getInPipeWriteFd()) // Input pipe error/hangup
        {
            // Check if output has already been handled
            bool outputHandled = false;
            for (size_t i = 0; i < clientsQueue.size(); ++i)
            {
                if (clientsQueue[i].fd == cgiIt->clientFd)
                {
                    outputHandled = true;
                    break;
                }
            }
            if (!outputHandled)
                handleCgiInputPipeError(fd, cgiIt);
            else
            {
                Logger::debug(std::string("Skipping input pipe error for FD ") + oss.str() + " as output already handled");
                cgiIt->cgi->closePipe(fd);
                poll_sets.erase(this->it);
            }
        }
        else
            Logger::warn(std::string("Unexpected CGI FD ") + oss.str() + " in poll error");
    }
    else // Client-related FD
        handleClientError(fd);
}

// Handles errors on CGI input pipes
void Webserver::handleCgiInputPipeError(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    std::ostringstream oss;
    oss << fd;
    Logger::error(std::string("CGI input pipe error on FD ") + oss.str());

    cgiIt->cgi->closePipe(cgiIt->fd); // Close the pipe
    int clientFd = cgiIt->clientFd;   // Get client FD
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Storing client FD ") + oss.str() + " for error response");

    // Clean up CGI object
    CGI* cgiPtr = cgiIt->cgi;
    cgiQueue.erase(cgiIt);
    oss.str(""); oss << fd;
    Logger::debug(std::string("Erased CGI tracker for FD ") + oss.str());
    delete cgiPtr;
    Logger::debug("Deleted CGI object");

    // Queue a 500 error response for the client
    std::string cgiResponse = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
    struct ClientTracker client = { clientFd, ClientRequest(), cgiResponse };
    clientsQueue.push_back(client);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added client FD ") + oss.str() + " to clientsQueue with 500 response");

    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLOUT;
    poll_sets.push_back(clientPoll);
    oss.str(""); oss << clientFd;
    Logger::debug(std::string("Added FD ") + oss.str() + " with POLLOUT to poll_sets");

    // Remove CGI FD from poll_sets
    for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); )
    {
        if (pit->fd == fd)
        {
            pit = poll_sets.erase(pit);
            oss.str(""); oss << fd;
            Logger::debug(std::string("Erased FD ") + oss.str() + " from poll_sets");
            break;
        }
        else
            ++pit;
    }

    oss.str(""); oss << clientFd;
    Logger::info(std::string("CGI error response queued for client FD ") + oss.str());
}

// Dispatches poll events to appropriate handlers
void Webserver::dispatchEvents()
{
    end = this->poll_sets.end();
    for (this->it = poll_sets.begin(); this->it != end; /* incrementing inside loop */)
    {
        std::ostringstream oss;
        oss << "FD " << this->it->fd << " revents: " << this->it->revents;
        Logger::debug(std::string("Checking ") + oss.str());

        if (this->it->revents & POLLIN) // Data to read
        {
            std::vector<CGITracker>::iterator cgiIt = retrieveCGI(this->it->fd);
            if (cgiIt != cgiQueue.end()) // CGI-related FD
            {
                if (this->it->fd == cgiIt->cgi->getOutPipeReadFd()) // CGI output
                {
                    handleCgiOutput(this->it->fd, cgiIt);
                    ++this->it;
                }
                else if (this->it->fd == cgiIt->cgi->getStderrFd()) // CGI stderr
                {
                    char buffer[256];
                    ssize_t bytes = read(this->it->fd, buffer, sizeof(buffer) - 1);
                    if (bytes > 0)
                    {
                        buffer[bytes] = '\0';
                        Logger::debug(std::string("CGI stderr: ") + std::string(buffer));
                        ++this->it;
                    }
                    else
                    {
                        Logger::debug(std::string("Stderr pipe closed or errored for FD ") + oss.str());
                        close(this->it->fd);
                        this->it = poll_sets.erase(this->it);
                    }
                }
            }
            else // Client or server socket
            {
                handlePollInEvent(this->it->fd);
                ++this->it;
            }
        }
        else if (this->it->revents & POLLOUT) // Ready to write
        {
            handlePollOutEvent(this->it->fd);
            ++this->it;
        }
        else if (this->it->revents & (POLLHUP | POLLERR | POLLNVAL)) // Error or hangup
        {
            Logger::debug(std::string("Detected POLLHUP, POLLERR, or POLLNVAL on FD ") + oss.str());
            handlePollErrorOrHangup(this->it->fd);
            if (this->it != poll_sets.end() && this->it->fd == fd) // Check if iterator is still valid
                ++this->it;
        }
        else if (this->it->revents != 0) // Unexpected event
        {
            Logger::warn(std::string("Unexpected revents value ") + oss.str());
            handlePollErrorOrHangup(this->it->fd);
            if (this->it != poll_sets.end() && this->it->fd == fd)
                ++this->it;
        }
        else // No events
        {
            Logger::debug(std::string("No events for FD ") + oss.str());
            ++this->it;
        }
    }
}

// Handles client disconnection during read
void Webserver::handleClientDisconnect(int fd, int bytesRecv)
{
    if (bytesRecv == 0) // Client closed connection
    {
        std::ostringstream oss;
        oss << fd;
        Logger::info(std::string("Client ") + oss.str() + " disconnected");
        close(fd);
        this->it = this->poll_sets.erase(this->it);
        if (retrieveClient(fd) != this->clientsQueue.end())
            this->clientsQueue.erase(retrieveClient(fd));
    }
}

// Processes a complete client request
void Webserver::processRequest(int fd, int contentLength)
{
    std::ostringstream oss;
    oss << this->totBytes;
    Logger::debug(std::string("recv this bytes: ") + oss.str());

    // Parse Content-Length from buffer if present
    size_t clPos = this->full_buffer.find("Content-Length: ");
    if (clPos != std::string::npos)
    {
        contentLength = atoi(this->full_buffer.substr(clPos + 16).c_str());
        oss.str(""); oss << contentLength;
        Logger::debug(std::string("Parsed Content-Length: ") + oss.str());
    }
    else
        contentLength = -1;

    // Check if we’ve received enough data to process
    if ((contentLength == -1 && this->totBytes > 0) || (contentLength > 0 && this->totBytes >= contentLength))
    {
        std::vector<struct ClientTracker>::iterator clientIt = retrieveClient(fd);
        if (clientIt == this->clientsQueue.end()) // Client not found
        {
            oss.str(""); oss << fd;
            Logger::error(std::string("Client not found ") + oss.str() + " - please Sveva review");
            return;
        }
        clientIt->request = ParsingRequest(this->full_buffer, this->totBytes); // Parse the request
        oss.str(""); oss << "Request URI: " << clientIt->request.getRequestLine()["Request-URI"];
        Logger::debug(oss.str());
        if (isCGI(clientIt->request.getRequestLine()["Request-URI"]) == true) // CGI request
            setupCGI(fd, clientIt);
        else // Static response
        {
            struct ClientTracker client;
            Logger::debug("fill in struct client");
            std::string response = prepareResponse(clientIt->request); // Generate response
            clientIt->response = response;
            client.fd = clientIt->fd;
            client.request = clientIt->request;
            client.response = clientIt->response;
            this->clientsQueue.erase(clientIt);
            this->clientsQueue.push_back(client);
            this->it->events = POLLOUT; // Switch to write mode
            this->full_buffer.clear();
            this->totBytes = 0;
            Logger::info("Response created successfully and stored in clientQueue");
        }
    }
    else
    {
        oss.str(""); oss << "Waiting for more data: " << this->totBytes << " of " << contentLength;
        Logger::debug(oss.str());
    }
}

// Sets up a CGI process for a client request
void Webserver::setupCGI(int fd, std::vector<struct ClientTracker>::iterator& clientIt)
{
    std::ostringstream oss;
    oss << "Entering setupCGI for FD " << fd;
    Logger::debug(oss.str());

    try
    {
        std::string body = this->full_buffer; // Raw request body
        std::cerr << "[DEBUG]: Full raw body size: " << body.size() << std::endl;

        // Log Content-Type and Content-Length for debugging
        std::string contentType = clientIt->request.getHeaders()["Content-Type"];
        std::string contentLengthStr = clientIt->request.getHeaders()["Content-Length"];
        int contentLength = atoi(contentLengthStr.c_str());
        oss.str(""); oss << "Setting CGI env - Content-Type: " << contentType << ", Content-Length: " << contentLength;
        Logger::debug(oss.str());

        // Preview first 200 bytes of body (replace non-printable chars with '.')
        std::string bodyPreview = body.substr(0, 200);
        for (size_t i = 0; i < bodyPreview.size(); ++i)
            if (!std::isprint(static_cast<unsigned char>(bodyPreview[i])))
                bodyPreview[i] = '.';
        Logger::debug(std::string("Full body preview (first 200 bytes): [") + bodyPreview + "]");

        // Create CGI object with request details
        CGI* cgi = new CGI(clientIt->request, _serverInfo->getServerRootPath() + "/uploads", *_serverInfo, body);
        CGITracker tracker = { cgi, cgi->getInPipeWriteFd(), clientIt->fd, body };
        cgiQueue.push_back(tracker);

        // Add CGI input pipe to poll_sets (POLLOUT)
        struct pollfd cgiInPoll;
        cgiInPoll.fd = cgi->getInPipeWriteFd();
        cgiInPoll.events = POLLOUT;
        poll_sets.push_back(cgiInPoll);

        // Add CGI output pipe to poll_sets (POLLIN)
        struct pollfd cgiOutPoll;
        cgiOutPoll.fd = cgi->getOutPipeReadFd();
        cgiOutPoll.events = POLLIN;
        poll_sets.push_back(cgiOutPoll);

        // Add CGI stderr pipe to poll_sets (POLLIN)
        struct pollfd cgiErrPoll;
        cgiErrPoll.fd = cgi->getStderrFd();
        cgiErrPoll.events = POLLIN;
        poll_sets.push_back(cgiErrPoll);

        oss.str(""); oss << "CGI setup for client FD " << fd << ", input FD " << cgiInPoll.fd << ", output FD " << cgiOutPoll.fd << ", stderr FD " << cgiErrPoll.fd;
        Logger::debug(oss.str());
        oss.str(""); oss << poll_sets.size();
        Logger::debug(std::string("Poll sets size after CGI setup: ") + oss.str());

        this->full_buffer.clear(); // Reset buffer
        this->totBytes = 0;
    }
    catch (const std::exception& e) // CGI setup failed
    {
        oss.str(""); oss << fd;
        Logger::error(std::string("CGI setup failed for FD ") + oss.str() + ": " + e.what());
        clientIt->response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == fd)
            {
                pit->events = POLLOUT; // Switch to write error response
                break;
            }
        }
        this->full_buffer.clear();
        this->totBytes = 0;
    }
}

// Reads and processes CGI output
void Webserver::handleCgiOutput(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    std::ostringstream oss;
    oss << "Read " << bytes << " from CGI FD " << fd;
    Logger::debug(oss.str());

    if (bytes > 0) // Data read successfully
    {
        cgiIt->cgi->appendOutput(buffer, bytes);
        oss.str(""); oss << "Appended " << bytes << " from CGI FD " << fd << " to CGI output";
        Logger::debug(oss.str());
    }
    else if (bytes == 0) // Pipe closed (CGI done)
    {
        std::string cgiOutput = cgiIt->cgi->getOutput();
        oss.str(""); oss << "CGI raw output: " << cgiOutput;
        Logger::debug(oss.str());

        // Queue response for client
        for (std::vector<ClientTracker>::iterator clientIt = clientsQueue.begin(); clientIt != clientsQueue.end(); ++clientIt)
        {
            if (clientIt->fd == cgiIt->clientFd)
            {
                clientIt->response = cgiOutput;
                break;
            }
        }
        oss.str(""); oss << "CGI response queued for client FD " << cgiIt->clientFd;
        Logger::info(oss.str());

        close(fd); // Close output pipe
        delete cgiIt->cgi;
        cgiQueue.erase(cgiIt);
        poll_sets.erase(it);

        // Switch client FD to POLLOUT
        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == cgiIt->clientFd)
            {
                pit->events = POLLOUT;
                break;
            }
        }
    }
    else // Read error
    {
        oss.str(""); oss << "Failed to read CGI output from FD " << fd;
        Logger::error(oss.str());
        close(fd);
        delete cgiIt->cgi;
        cgiQueue.erase(cgiIt);
        poll_sets.erase(it);
    }
}

// Handles reading from client sockets
void Webserver::handleReadEvents(int fd)
{
    std::string response;
    int contentLength = -1;
    int bytesRecv = readData(fd, this->full_buffer, this->totBytes);
    std::ostringstream oss;
    oss << "Read " << bytesRecv << " bytes from FD " << fd;
    Logger::debug(oss.str());
    if (bytesRecv == 0) // Client disconnected
        handleClientDisconnect(fd, bytesRecv);
    else
    {
        // Parse Content-Length if present
        size_t clPos = this->full_buffer.find("Content-Length: ");
        if (clPos != std::string::npos)
        {
            contentLength = atoi(this->full_buffer.substr(clPos + 16).c_str());
            oss.str(""); oss << contentLength;
            Logger::debug(std::string("Content-Length from header: ") + oss.str());
        }
        // Process if enough data received
        if ((contentLength > 0 && this->totBytes >= contentLength) || (contentLength == -1 && bytesRecv > 0))
            processRequest(fd, contentLength);
        else if (bytesRecv < 0 && this->totBytes > 0)
            processRequest(fd, contentLength);
        else
            Logger::debug(std::string("No data or error on FD ") + oss.str() + ", waiting for next poll");
    }

    // Check for CGI output
    std::vector<CGITracker>::iterator cgiIt = retrieveCGI(fd);
    if (cgiIt != cgiQueue.end() && cgiIt->fd == cgiIt->cgi->getOutPipeReadFd())
        handleCgiOutput(fd, cgiIt);
}

// Writes input to CGI stdin
void Webserver::writeCGIInput(int fd, std::vector<CGITracker>::iterator& cgiIt)
{
    if (cgiIt != cgiQueue.end() && cgiIt->fd == cgiIt->cgi->getInPipeWriteFd())
    {
        std::ostringstream oss;
        oss << "Writing to CGI input FD " << fd;
        Logger::debug(oss.str());
        cgiIt->cgi->writeInput(); // Write request body to CGI
        if (cgiIt->cgi->isInputDone()) // Input complete
        {
            int outFd = cgiIt->cgi->getOutPipeReadFd();
            this->it->fd = outFd;
            this->it->events = POLLIN; // Switch to reading output
            cgiIt->fd = outFd;
            oss.str(""); oss << outFd;
            Logger::debug(std::string("Switched to CGI output FD ") + oss.str() + " with POLLIN");

            // Log the full body sent to CGI
            std::string bodyPreview = cgiIt->input_data;
            oss.str(""); oss << cgiIt->input_data.size();
            for (size_t i = 0; i < bodyPreview.size(); ++i)
                if (!std::isprint(static_cast<unsigned char>(bodyPreview[i])))
                    bodyPreview[i] = '.';
            Logger::debug(std::string("Full body sent to CGI (size ") + oss.str() + "): [" + bodyPreview + "]");
        }
    }
    else
    {
        std::ostringstream oss;
        oss << fd;
        Logger::debug(std::string("No CGI match for FD ") + oss.str());
    }
}

// Sends response to client
void Webserver::sendClientResponse(int fd, std::vector<struct ClientTracker>::iterator& iterClient)
{
    std::ostringstream oss;
    oss << fd;
    Logger::debug(std::string("Sending response for FD ") + oss.str());
    oss.str(""); oss << iterClient->response.size();
    Logger::debug(std::string("bytes to send ") + oss.str());

    if (iterClient->response.empty()) // No response to send
    {
        Logger::debug(std::string("No response to send for FD ") + oss.str());
        close(fd);
        this->it = poll_sets.erase(this->it);
        clientsQueue.erase(iterClient);
        return;
    }

    // Send response non-blocking
    int bytes = send(fd, iterClient->response.c_str(), iterClient->response.size(), MSG_DONTWAIT);
    if (bytes == -1) // Send failed
    {
        Logger::error(std::string("Failed send for FD ") + oss.str() + ": " + std::string(strerror(errno)));
        close(fd);
        this->it = poll_sets.erase(this->it);
        clientsQueue.erase(iterClient);
    }
    else // Send succeeded
    {
        oss.str(""); oss << bytes;
        Logger::info(std::string("these bytes were sent ") + oss.str());
        oss.str(""); oss << fd;
        for (std::vector<struct pollfd>::iterator pit = poll_sets.begin(); pit != poll_sets.end(); ++pit)
        {
            if (pit->fd == fd)
            {
                pit->events = POLLIN; // Switch back to reading
                Logger::debug(std::string("Switched FD ") + oss.str() + " to POLLIN after sending");
                break;
            }
        }
        clientsQueue.erase(iterClient);
    }
}

// Parses raw HTTP request into a ClientRequest object
ClientRequest Webserver::ParsingRequest(std::string str, int size)
{
    ClientRequest request;
    request.parseRequestHttp(str, size); // Parse the raw string
    return request;
}

// Prepares a response for non-CGI requests
std::string Webserver::prepareResponse(ClientRequest request)
{
    std::string response;
    std::map<std::string, std::string> httpRequestLine = request.getRequestLine();
    if (httpRequestLine.find("Method") != httpRequestLine.end()) // Valid method
    {
        ServerResponse serverResponse(request, *this->_serverInfo);
        if (httpRequestLine["Method"] == "GET")
            response = serverResponse.responseGetMethod();
        else if (httpRequestLine["Method"] == "POST")
            response = serverResponse.responsePostMethod();
        else if (httpRequestLine["Method"] == "DELETE")
            response = serverResponse.responseDeleteMethod();
        else
            Logger::error("Method not found, Sveva, use correct status code line");
    }
    else
        Logger::error("Method not found, Sveva, use correct status code line");
    return response;
}

//---------------------------------Helpers-----------------------------

// Checks if an FD is a server socket
int Webserver::fdIsServerSocket(int fd)
{
    int size = this->serverFds.size();
    for (int i = 0; i < size; i++)
        if (fd == this->serverFds[i])
            return true;
    return false;
}

// Adds server sockets to poll_sets for monitoring
void Webserver::addServerSocketsToPoll(std::vector<int> fds)
{
    struct pollfd serverPoll[MAX];
    int clientsNumber = (int)fds.size();
    for (int i = 0; i < clientsNumber; i++)
    {
        serverPoll[i].fd = fds[i];
        serverPoll[i].events = POLLIN; // Watch for incoming connections
        this->poll_sets.push_back(serverPoll[i]);
    }
    Logger::info("Add server sockets to poll sets");
}

// Accepts a new client connection
void Webserver::createNewClient(int fd)
{
    socklen_t clientSize;
    struct sockaddr_storage clientStruct;
    struct pollfd clientPoll;
    int clientFd;

    clientSize = sizeof(clientStruct);
    clientFd = accept(fd, (struct sockaddr *)&clientStruct, &clientSize); // Accept new client
    if (clientFd == -1)
    {
        Logger::error(std::string("Failed to create new client (accept): ") + std::string(strerror(errno)));
        return;
    }
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN; // Watch for data from client
    this->poll_sets.push_back(clientPoll);
    struct ClientTracker newClient;
    newClient.fd = clientFd;
    this->clientsQueue.push_back(newClient);
    std::ostringstream oss;
    oss << clientFd;
    Logger::info(std::string("New client ") + oss.str() + " created and added to poll sets");
}

// Finds a client in clientsQueue by FD
std::vector<struct ClientTracker>::iterator Webserver::retrieveClient(int fd)
{
    for (std::vector<struct ClientTracker>::iterator iterClient = this->clientsQueue.begin(); iterClient != clientsQueue.end(); ++iterClient)
        if (iterClient->fd == fd)
            return iterClient;
    return this->clientsQueue.end();
}

// Finds a CGI tracker in cgiQueue by FD
std::vector<struct CGITracker>::iterator Webserver::retrieveCGI(int fd)
{
    for (std::vector<struct CGITracker>::iterator iterCGI = this->cgiQueue.begin(); iterCGI != cgiQueue.end(); ++iterCGI)
        if (iterCGI->fd == fd)
            return iterCGI;
    return this->cgiQueue.end();
}

// Reads data from a socket into a buffer
int Webserver::readData(int fd, std::string &str, int &bytes)
{
    int res = 0;
    char buffer[BUFFER];
    std::ostringstream oss;
    int contentLength = -1;
    size_t clPos = str.find("Content-Length: ");
    if (clPos != std::string::npos)
    {
        contentLength = atoi(str.substr(clPos + 16).c_str());
        oss.str(""); oss << contentLength;
        Logger::debug(std::string("Content-Length from header: ") + oss.str());
    }

    do
    {
        memset(buffer, 0, sizeof(buffer));
        res = recv(fd, buffer, BUFFER - 1, MSG_DONTWAIT); // Non-blocking read
        oss.str(""); oss << "recv returned " << res << " for FD " << fd;
        Logger::debug(oss.str());
        if (res > 0) // Data received
        {
            str.append(buffer, res);
            bytes += res;
            oss.str(""); oss << bytes;
            Logger::debug(std::string("Total bytes read: ") + oss.str());
        }
        else if (res == 0) // Client closed connection
        {
            oss.str(""); oss << fd;
            Logger::info(std::string("Client FD ") + oss.str() + " closed connection");
            break;
        }
        else // No data or error
        {
            oss.str(""); oss << fd;
            Logger::debug(std::string("recv done or no data yet for FD ") + oss.str());
            break;
        }
    } while (res > 0 && (contentLength == -1 || bytes < contentLength)); // Continue until content length met

    oss.str(""); oss << "Final buffer size: " << str.size();
    Logger::debug(oss.str());
    return res;
}

// Checks if a file exists at the given path
int Webserver::searchPage(std::string path)
{
    FILE *folder = fopen(path.c_str(), "rb");
    if (folder == NULL)
        return false;
    fclose(folder);
    return true;
}

// Determines if a request URI requires CGI processing
int Webserver::isCGI(std::string str)
{
    if (str == "/upload" || str.find(".py") != std::string::npos) // CGI triggers
        return true;
    if (searchPage(this->_serverInfo->getServerDocumentRoot() + str) == true) // Static file exists
        return false;
    return false;
}

// Closes all sockets in poll_sets
void Webserver::closeSockets()
{
    int size = (int)this->poll_sets.size();
    for (int i = 0; i < size; i++)
        if (this->poll_sets[i].fd >= 0)
            close(this->poll_sets[i].fd);
}