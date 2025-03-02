//////////////////////////
// CGI INPUT AND OUTPUT //
//////////////////////////

CGI INPUT: (Server to cgi)

	CGITracker::input_data: what the server sends to CGI script-s stdin 
	(in Sveva's bit: data_input)

Request Data: client's request body, e.g. POST data, stored in tracker.input_data, 
to write in _in_pipe[1] via handleCGIInput


Note> The CGI Output is NOT in the CGITracker

--------------------------------------------------------------------

CGI OUTPUT: (Cgi to server)

	CGI::_output

CGI’s reply (e.g., "Content-Type: text/plain\r\n\r\nReceived: data=example") goes into CGI::_output, read from _out_pipe[0].

----------------------------------------------------------------------

 
///////////////////////////
/// The Role of the CGI 
//////////////////////////
 
 - When a .py request comes in, CGI spawns a child process (e.g., Python script) 
 and sets up two pipes:
 	- _in_pipe[1]: Server writes request data (e.g., POST body) to the script’s stdin.
 	- _out_pipe[0]: Server reads the script’s stdout (e.g., HTTP response).
 	
- The CGI pipes create FDs (e.g., 5 for _in_pipe[1], 6 for _out_pipe[0]).
- The CGI needs to integrate these FDs into poll_sets for I/O.


/////////////////////////////////////////////////////////////
//
// CGI FLOW
//
// how CGI interacts with poll_sets in the updated version
//  
//////////////////////////////////////////////////////////
 
 SHORT VERSION OF THE CGI FLOW
 
CGI interacts with pollsets:
	handleCGIRequest: Adds CGI FDs to poll_sets.
	poll(): Watches for POLLOUT (write) and POLLIN (read).
	dispatchEvents: Routes events via cgi_map.
	handleCGIInput/handleCGIOutput: Manages I/O, updates poll_sets.
	cleanupCGI: Removes FDs, sends response.


How data moves from a client's POST request through CGI and back to the client:

	POST Flow: data_input → _in_pipe[1] → CGI stdin → CGI stdout → _out_pipe[0] → _output → \
	client.

 
 ----------------------------------------------------------------------------

------------------------------------------------------------------------------

 LONG VERSION OF THE CGI FLOW
 
1. Request Arrives (from Client)

	handleClientSocketEvent
		reads request into full_buffer via readData
		calls serverParsingAndResponse

	
2. CGI Detection and Setup

	serverParsingAndResponse
		parseRequest
		sees .py --> sends to handleCGIRequest

	handleCGIRequest
		creates CGI instance
		CGI::runScript() --> creates _in_pipe[1] (say,FD 5) and _out_pipe[0] (say, FD 6)

 	Tracking
		CGITracker writes client's and input data in the tracker
			cgi_map[cgi->getInPipeWriteFd()] = tracker: maps FD 5.
			cgi_map[cgi->getOutPipeReadFd()] = tracker: maps FD 6.
			
	Add to poll_sets:
		
		if (!tracker.input_data.empty()) 
		{  // POST has data
    			struct pollfd in_fd = {cgi->getInPipeWriteFd(), POLLOUT, 0};
    			poll_sets.push_back(in_fd);  // FD 5, POLLOUT
		}
		struct pollfd out_fd = {cgi->getOutPipeReadFd(), POLLIN, 0};
		poll_sets.push_back(out_fd);  // FD 6, POLLIN
		
	FD 5: Watches for POLLOUT, ready to write "data=example".
	FD 6: Watches for POLLIN, ready to read CGI output.


3. poll() Monitors FDs

	runPollLoop
		poll call : check all FDs
		poll_sets now includes:
			FD3 (client_fd, POLLIN, clinet socket),
			FD5 (_in_pipe[1], POLLOUT, CGI input)
			FD6 (_out_pipe[0], POLLIN, CGI output).
	poll sets revents (e.g. POLLOUT on FD 5 (pipe writeable), later POLLIN on FD 6 (output ready).
	
4. Handling CGI Events

	dispatchEvents 
		     -  loops through poll_sets
		     -  checks event occurred: it->revents & (POLLIN | POLLOUT)
		     -  CGI FD check:
		       		if (cgi_map.find(it->fd) != cgi_map.end()) 
		       		{
    					CGITracker& tracker = cgi_map[it->fd];
    					if (it->revents & POLLOUT && it->fd == tracker.cgi\
    					>getInPipeWriteFd()) 
					        handleCGIInput(tracker, it);
				}
    				else if (it->revents & POLLIN && it->fd == tracker.cgi-\
    				>getOutPipeReadFd()) 
        				handleCGIOutput(tracker, it);
    				else
        				++it;
        		
        	     -FD5 POLLOUT: 
        	     		handleCGIInput writes "data=example" to FD5
        	     		updates _bytes_written; if done, _input_done=true erases FD4 from\ 			
        	     		poll_sets
        	     		
        	     -FD6 POLLIN:
        	     		handleCGIOutput reads output 
        	     		(e.g. "Content-Type: text/plain\r\n\r\nReceived: data=example") 
        	     		into CGI::_output.
    				If EOF (bytes_read == 0), _output_done = true.
				If isDone(), calls cleanupCGI.

5. Cleanup and Response
	cleanupCGI
		- std::string response = cgi->getOutput();—gets 
		  "Content-Type: text/plain\r\n\r\nReceived: data=example".
		- queueResponse(tracker.client_fd, response, "CGI"): queues to FD 3.
		- Removes FD 5 and FD 6 from cgi_map and poll_sets.
	handleSendResponse
		- poll() signals POLLOUT on FD3
		- send(it->fd, response.c_str(), response.size(), 0)—sends to client.
		
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////
// CGI’s Interaction with poll_sets //
//////////////////////////////////////

- Direct Addition: handleCGIRequest adds _in_pipe[1] and _out_pipe[0] to poll_sets directly.

- Event Trigger: poll() monitors these FDs—POLLOUT for input, POLLIN for output.

- Server-Driven: the ServerSocket class handles all the polling logic. The CGI class just provides FDs via getInPipeWriteFd() and getOutPipeReadFd().




/////////////////////////////////////////////////
// The point of CGI tracker: FILTER FOR SERVER //
/////////////////////////////////////////////////

By itself, poll_sets mixes all FDs: server sockets, client sockets, CGI pipes. 

BUT when poll() signals an event (e.g., POLLIN on FD 6), the server needs to know 
if it’s a CGI FD and which one.

.py detection starts CGI, but cgi_map manages the I/O and response routing.


KEY POINT: cgi_map ACTS AS A FILTER

	(1) dispatchEvents checks cgi_map.find(it->fd): if found, it’s a CGI FD.

	(2) then if checks if this is input or output:
			i.e. it checks it->fd == tracker.cgi->getInPipeWriteFd(), 
			or getOutPipeReadFd()

One can think of the cgi_map as a dictionary that maps CGI pipe FDs to their context:
(i) CGI instance, (ii) client FD, and (iii) input data.


ADDING TO THE CGI_MAP: handleCGIRequest adds FDs to cgi_map

	cgi_map[cgi->getInPipeWriteFd()] = tracker;  // e.g., cgi_map[5]
	cgi_map[cgi->getOutPipeReadFd()] = tracker;  // e.g., cgi_map[6]

SUBTRACTING FROM THE CGI_MAP: cleanUpCGI removse FDs from cgi_map

	cgi_map.erase(cgi->getInPipeWriteFd());
	cgi_map.erase(it->fd);  // it->fd is typically _out_pipe[0]

//////////////////////////////////////////////

////////////////////////////////////////////
// WHAT ULTIMATELY THE CGI SENDS TO Sveva //
////////////////////////////////////////////

The output of the CGI process that the server receives is ultimately a std::string

This is the HTTP response (headers and body) that the CGI script writes to its stdout, 
and which the server reads and buffers into CGI::_output.

Who makes it? The CGI script 

/// MORE DETAILS ON (CURRENT) OUTPUT
// WHAT HAPPENS IN RUNSCRIPT
//
// dup2(_out_pipe[1], STDOUT_FILENO): 
//  redirects the CGI script’s stdout (FD 1) to _out_pipe[1] (e.g., FD 6 in the child)
// Script’s print() writes "Content-Type: text/plain\r\n\r\nReceived: 
//          "data=example" to _out_pipe[1].
// Server keeps _out_pipe[0] (e.g., FD 6 in parent) and reads from it.


// AFTER THAT: SERVER READS OUTPUT
//
// Server Reads Output: Server::handleCGIOutput
//      it->fd is _out_pipe[0] (e.g., FD 6): poll() signals POLLIN.
//      read() fetches the string into buffer, e.g., 
//      "Content-Type: text/plain\r\n\r\nReceived: data=example".
// Might read in chunks if large—buffer is 1024 bytes max.
// cgi->appendOutput(buffer, bytes_read) builds CGI::_output
//
// Server Stores Output as a String
// 
// CGI::_output holds the full CGI output as a std::string
// e.g., "Content-Type: text/plain\r\n\r\nReceived: data=example".
// 
// WHY A STRING?
// read() gives char[]—appendOutput converts to std::string for easy handling.
//
//

// AFTER THAT: SERVER SENDS TO CLIENT
// Server::cleanupCGI
// getOutput() returns CGI::_output, namely the string which is our response.
// 
// queueResponse adds it to response_queue: response is "Content-Type.....etc"
// 
// handleSendResponse sends it to client_fd (e.g., FD 3) via send().

// Potential: CGI could output binary (e.g., images), but our current setup treats it as a string (\0-terminated)
// For binary, we’d need std::vector<char>.

