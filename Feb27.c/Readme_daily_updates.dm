
TO DO: 28 Feb 
	- Testing
	- CGI scripts

///////////////////////////////////////////////////


27 Feb

1. Changed CGI so that it writes into poll_sets directly.
   (removed getPollFds and initPollFds functions: leaving fd-handling directly to SocketServer)
2. Written the updated flow in a CGI Flow readme file
3. Substituded Server with InfoServer (for testing purposes)


////////////////////////////////////


26 Feb

1. Passing Server to CGI and ServerSocket classes, instead of InfoServer
2. Included some forward declarations in CGI and in ServerSocket 
   (one class by H - Server - and two by Sveva -- ClientResponse and ServerResponse)
3. Changes to HandleCGI (and maybe another ft too) in ServerSocket 
4. Made many changes to CGI to get variables from the parsing people


///////////////////////////////////////////////////


25 Feb

//-------------------------------------------------------------------------------------
//      NOTE the function sendResponse() might need to be modified
//      LOCATION: ServerSocket
//-------------------------------------------------------------------------------------

// THE sendResponse() FUNCTION:

void Server::sendResponse(int fd, const std::string& response, const std::string& method) 
{
    if (send(fd, response.c_str(), strlen(response.c_str()), 0) == -1) 
    {
        std::cerr << "Failed to send response to client " << fd << " for " << method << ": " << strerror(errno) << std::endl;
        printError(SEND);
    } 
    else 
        std::cout << "Done with " << (method.empty() ? "error" : method) << " response for client " << fd << std::endl;
}


// WORRY ABOUT sendResponse(): blocking
// send() without POLLOUT violates the non-blocking rule set in the subject

// Blocking Risk: send() is blocking, and in sendResponse() it
// isn’t called directly in the poll() loop: post-poll() processing. 

// The subject says: “You must never do a read or a write operation without 
// going through poll() (or equivalent),” and send() (a write operation) 
// isn’t poll()-gated so far.

// Diagnosis: send() needs poll() protection.


// ----------------------------------------------------------------------------

// ALTERNATIVE VERSION> queueResponseResponse()
// queueResponse() queues responses and sends via poll() with POLLOUT.
// The "Fully_Updated_version" implements ServerSocket with queueResponse(), instead of sendResponse()


