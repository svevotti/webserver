#include "CGIHandler.hpp"
#include "ClientHandler.hpp"
#include "HttpRequest.hpp"
#include "Utils.hpp"
#include <fstream>
//NEW: Simona, missing this library maybe?
#include <signal.h>


CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler()
{
	std::vector<CGITracker>::iterator it;
	for (it = _cgiQueue.begin(); it != _cgiQueue.end(); ++it)
	{
		// Close any open pipes before deleting
		if (it->cgi->getInPipeWriteFd() > 0)
			close(it->cgi->getInPipeWriteFd());
		if (it->cgi->getOutPipeReadFd() > 0)
			close(it->cgi->getOutPipeReadFd());
		if (it->cgi->getErrPipeReadFd() > 0)
			close(it->cgi->getErrPipeReadFd());
		delete it->cgi;
	}
	_cgiQueue.clear();
}

void CGIHandler::determineErrorResponse(const std::string& cgi_path, int statusCode,
	std::string& errorBody, std::string& contentType)
{
	bool isUploadPy = cgi_path.find("upload.py") != std::string::npos;
	bool isLoopPy = cgi_path.find("loop.py") != std::string::npos;

	// Define status text map
	std::map<int, std::string> statusTexts;
	statusTexts[400] = "Bad Request";
	statusTexts[401] = "Unauthorized";
	statusTexts[403] = "Forbidden";
	statusTexts[404] = "Not Found";
	statusTexts[405] = "Method Not Allowed";
	statusTexts[413] = "Payload Too Large";
	statusTexts[415] = "Unsupported Media Type";
	statusTexts[500] = "Internal Server Error";
	statusTexts[501] = "Not Implemented";
	statusTexts[502] = "Bad Gateway";
	statusTexts[503] = "Service Unavailable";
	statusTexts[504] = "Gateway Timeout";
	statusTexts[505] = "HTTP Version Not Supported";

	std::string statusText = statusTexts[statusCode];
	if (statusText.empty())
		statusText = "Unknown Error";

	if (isUploadPy)
	{
		// For upload.py, return JSON error
		std::ostringstream json;
		json << "{\"error\": \"" << statusText << "\", \"code\": " << statusCode << "}";
		errorBody = json.str();
		contentType = "application/json";
	}
	else if (isLoopPy)
	{
		// For loop.py, return plain text
		std::ostringstream text;
		text << statusCode << " " << statusText;
		errorBody = text.str();
		contentType = "text/plain";
	}
	else
	{
		// For other scripts, return HTML
		std::ostringstream html;
		html << "<!DOCTYPE html><html><body><h1>" << statusCode << " "
		<< statusText << "</h1><p>"
		<< "The server cannot process this request." << "</p></body></html>";
		errorBody = html.str();
		contentType = "text/html";
	}
}

void CGIHandler::startCGI(ClientHandler& client, const InfoServer& serverConfig, std::vector<struct pollfd>& poll_sets)
{
	Logger::debug("Starting CGI for FD " + Utils::toString(client.getFd()) + " URI: " + client.getRequest().getHttpRequestLine()["request-uri"]);
	Logger::debug("Server config: IP=" + serverConfig.getIP() + ", Port=" + serverConfig.getPort() + ", cgi_processing_timeout=" + Utils::toString(serverConfig.getCGIProcessingTimeout()));
	HttpRequest request = client.getRequest();
	CGITracker tracker;
	tracker.cgi = NULL;

	std::string requestUri = request.getHttpRequestLine()["request-uri"];
	std::string method = request.getMethod();
	std::string uploadDir;

	std::map<std::string, Route> routes = serverConfig.getRoute();
	std::string routeKey = client.findDirectory(requestUri);
	std::map<std::string, Route>::const_iterator routeIt = routes.find(routeKey);

	// Fix path construction based on root configuration
	std::string cgi_path;
	if (serverConfig.getRoot().empty()) // Default to current directory if root is empty
		cgi_path = "." + requestUri;
	else if (serverConfig.getRoot()[0] == '/')         // If root starts with '/', it's likely a relative path that needs adjustment
		cgi_path = "." + serverConfig.getRoot() + requestUri;
	else // Otherwise use as configured
		cgi_path = "./" + serverConfig.getRoot() + requestUri;

	// Check if this is an upload script
	bool isUploadPy = cgi_path.find("upload.py") != std::string::npos;

	Logger::debug("CGI path details - Full path: " + cgi_path);
	Logger::debug("CGI path details - Server root: " + serverConfig.getRoot());
	Logger::debug("CGI path details - Request URI: " + requestUri);
	Logger::debug("CGI path details - Does file exist: " + std::string(access(cgi_path.c_str(), F_OK) == 0 ? "Yes" : "No"));
	Logger::debug("CGI path details - Is file executable: " + std::string(access(cgi_path.c_str(), X_OK) == 0 ? "Yes" : "No"));

	// Payload size checks
	std::string contentLengthStr = request.getContentLength();
	size_t contentLength = contentLengthStr.empty() ? 0 : Utils::toInt(contentLengthStr);
	size_t maxSize = 10 * 1024 * 1024; // Default 10MB
	std::map<std::string, std::string> settings = serverConfig.getSetting();
	if (settings.find("client_max_body_size") != settings.end())
		maxSize = Utils::toInt(settings.find("client_max_body_size")->second);

	// 413 Check for payload size exceeding limits
	if (contentLength > maxSize)
	{
		std::string errorBody, contentType;
		determineErrorResponse(cgi_path, 413, errorBody, contentType);
		client.setResponse(createStandardResponse(413, errorBody, contentType, true).composeResponse());
		addPipeToPoll(client.getFd(), POLLOUT, poll_sets);
		Logger::error("413 Payload Too Large: Request body exceeds configured limit");
		return;
	}

	// 415 Check for content type validation for uploads
	if (isUploadPy && request.getMethod() == "POST")
	{
		// Use the map of headers instead of getHeaderValue
		std::string contentType = "";
		std::map<std::string, std::string> headers = request.getHttpHeaders();
		if (headers.find("content-type") != headers.end())
			contentType = headers["content-type"];

		if (contentType.find("multipart/form-data") == std::string::npos)
		{
			std::string errorBody, errorContentType;
			determineErrorResponse(cgi_path, 415, errorBody, errorContentType);

			HttpResponse response(415, errorBody);
			response.setHeader("Content-Type", errorContentType);
			client.setResponse(response.composeResponse());
			addPipeToPoll(client.getFd(), POLLOUT, poll_sets);
			Logger::error("415 Unsupported Media Type: Upload requires multipart/form-data");
			return;
		}
	}

	// Check if the CGI script exists - 404 Not Found otherwise
	if (access(cgi_path.c_str(), F_OK) != 0)
	{
		std::string errorBody, contentType;
		determineErrorResponse(cgi_path, 404, errorBody, contentType);
		// Create a 404 response
		client.setResponse(createStandardResponse(404, errorBody, contentType, true).composeResponse());
		addPipeToPoll(client.getFd(), POLLOUT, poll_sets);
		Logger::error("404 Not Found: CGI script " + cgi_path + " does not exist");
		return;
	}

	// Check if the CGI script is executable - 403 Forbidden otherwise
	if (access(cgi_path.c_str(), X_OK) != 0)
	{
		std::string errorBody, contentType;
		determineErrorResponse(cgi_path, 403, errorBody, contentType);
		// Create a 403 response
		client.setResponse(createStandardResponse(403, errorBody, contentType, true).composeResponse());
		addPipeToPoll(client.getFd(), POLLOUT, poll_sets);
		Logger::error("403 Forbidden: CGI script " + cgi_path + " is not executable");
		return;
	}

	// Check if the CGI extension is supported
	std::string extension = cgi_path.substr(cgi_path.find_last_of("."));
	if (routeIt != routes.end())
	{
		bool isAllowed = false;

		// Check CGI extension using Route's cgi_pass mapping
		if (routeIt->second.locSettings.find("cgi_pass") != routeIt->second.locSettings.end())
		{
			std::string cgiPass = routeIt->second.locSettings.find("cgi_pass")->second;
			// Check if the extension is in the cgi_pass mapping
			if (cgiPass.find(extension) != std::string::npos)
				isAllowed = true;
		}

		// Also check if there's a direct cgi_extension setting
		if (routeIt->second.locSettings.find("cgi_extension") != routeIt->second.locSettings.end())
		{
			std::string cgiExts = routeIt->second.locSettings.find("cgi_extension")->second;
			if (cgiExts.find(extension) != std::string::npos)
				isAllowed = true;
		}

		if (!isAllowed) // 501 Not Implemented if extension is not allowed
		{
			std::string errorBody, contentType;
			determineErrorResponse(cgi_path, 501, errorBody, contentType);
			// Create a 501 response
			client.setResponse(createStandardResponse(501, errorBody, contentType, true).composeResponse());
			addPipeToPoll(client.getFd(), POLLOUT, poll_sets);
			Logger::error("501 Not Implemented: CGI extension " + extension + " not supported");
			return;
		}
	}

	// Determine upload directory based on method and route settings
	if (method == "POST")
	{
		if (routeIt != routes.end() && routeIt->second.locSettings.find("upload_to") != routeIt->second.locSettings.end())
		{
			// Make sure we apply the same path resolution logic to uploadDir
			std::string uploadPath = routeIt->second.locSettings.find("upload_to")->second;
			if (serverConfig.getRoot()[0] == '/')
				uploadDir = "." + serverConfig.getRoot() + uploadPath;
			else
				uploadDir = "./" + serverConfig.getRoot() + uploadPath;
		}
		else
			uploadDir = "./www/upload"; // Default for POST
	}

	Logger::debug("Upload dir for Server " + serverConfig.getIP() + ":" + serverConfig.getPort() + ": " + (uploadDir.empty() ? "none" : uploadDir));
	Logger::debug("Upload dir determined: " + (uploadDir.empty() ? "none" : uploadDir));

	// Check client_max_body_size before CGI execution
	std::string settingsDebug = "Server settings: ";
	for (std::map<std::string, std::string>::const_iterator it = settings.begin(); it != settings.end(); ++it)
		settingsDebug += it->first + "=" + it->second + ", ";
	Logger::debug(settingsDebug);

	try
	{
		// Pass server-specific configuration to CGI
		tracker.cgi = new CGI(request, uploadDir, serverConfig);

		// debugging
		Logger::debug("Created CGI object with pipes: in=" + Utils::toString(tracker.cgi->getInPipeWriteFd()) +
		", out=" + Utils::toString(tracker.cgi->getOutPipeReadFd()) +
		", err=" + Utils::toString(tracker.cgi->getErrPipeReadFd()));

		// Pass client_max_body_size to CGI environment
		if (settings.find("client_max_body_size") != settings.end())
		{
			std::string maxSizeStr = settings.find("client_max_body_size")->second;
			setenv("CLIENT_MAX_BODY_SIZE", maxSizeStr.c_str(), 1);
			Logger::debug("Set CLIENT_MAX_BODY_SIZE=" + maxSizeStr + " for CGI");
		}

		// Store server configuration in tracker for future reference
		tracker.serverConfig = &serverConfig;
		tracker.clientFd = client.getFd();
		tracker.pipeFd = tracker.cgi->getInPipeWriteFd();
		tracker.response = "";
		tracker.isActive = true; // Mark this CGI process as active
		tracker.firstResponseSent = false;

	}
	catch (const CGIException& e)
	{
		Logger::error("CGI initialization failed: " + std::string(e.what()));

		// Clean up the CGI object if it was created
		if (tracker.cgi)
		{
			delete tracker.cgi;
			tracker.cgi = NULL;
		}

		std::string errorBody, contentType;
		determineErrorResponse(cgi_path, 500, errorBody, contentType);
		// Create a 500 response
		client.setResponse(createStandardResponse(500, errorBody, contentType, true).composeResponse());
		addPipeToPoll(client.getFd(), POLLOUT, poll_sets);// Add the client to poll set for writing
		return;
	}

	// Add pipe file descriptors to poll set
	addPipeToPoll(tracker.cgi->getInPipeWriteFd(), POLLOUT, poll_sets);
	addPipeToPoll(tracker.cgi->getOutPipeReadFd(), POLLIN, poll_sets);
	addPipeToPoll(tracker.cgi->getErrPipeReadFd(), POLLIN, poll_sets);

	_cgiQueue.push_back(tracker);

	// Logging
	Logger::info("CGI started for Server " + serverConfig.getIP() + ":" + serverConfig.getPort() +
		": clientFd=" + Utils::toString(tracker.clientFd) +
		", inputFd=" + Utils::toString(tracker.cgi->getInPipeWriteFd()) +
		", outputFd=" + Utils::toString(tracker.cgi->getOutPipeReadFd()) +
		", errorFd=" + Utils::toString(tracker.cgi->getErrPipeReadFd()) +
		", uri=" + requestUri +
		", uploadDir=" + (uploadDir.empty() ? "none" : uploadDir) +
		", clientTimeout=" + Utils::toString(client.getTimeOut()));
}

void CGIHandler::handleCGIOutput(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
	std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
	if (cgiIt == _cgiQueue.end())
	{
		Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
		for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end(); ++it) {
			if (it->fd == fd)
			{
				poll_sets.erase(it);
				break;
			}
		}
		return;
	}

	// Get FDs before any potential closing
	int fdsToClose[3] = { cgiIt->cgi->getInPipeWriteFd(), cgiIt->cgi->getOutPipeReadFd(), cgiIt->cgi->getErrPipeReadFd() };
	Logger::debug("FDs before timeout check: in=" + Utils::toString(fdsToClose[0]) +
				", out=" + Utils::toString(fdsToClose[1]) +
				", err=" + Utils::toString(fdsToClose[2]));

	// Check if the CGI process has timed out
	cgiIt->cgi->killIfTimedOut();

	// Handle timeout or completed CGI
	// if (cgiIt->cgi->isTimedOut() || cgiIt->cgi->isOutputDone())
	// {
	//     Logger::warn("CGI FD " + Utils::toString(fd) + (cgiIt->cgi->isTimedOut() ? " timed out and killed" : " completed"));
	//     std::string finalChunk = generateCGIResponse(cgiIt, "", true);
	//     setClientResponse(cgiIt, poll_sets, clients, finalChunk);
	//     Logger::debug("Before cleanup: in=" + Utils::toString(fdsToClose[0]) +
	//                  ", out=" + Utils::toString(fdsToClose[1]) +
	//                  ", err=" + Utils::toString(fdsToClose[2]));
	//     removeCGIFDsFromPollSets(poll_sets, fdsToClose);
	//     closeCGIFDs(fdsToClose, cgiIt);
	//     removeCGI(cgiIt);
	//     Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
	//     return;
	// }


	// Handle timeout
	if (cgiIt->cgi->isTimedOut())
	{
		Logger::warn("CGI FD " + Utils::toString(fd) + " timed out and killed");
		// Generate error chunk via generateCGIResponse instead of a new 503 response
		std::string errorChunk = generateCGIResponse(cgiIt, "", true);
		if (!errorChunk.empty())
		{
			setClientResponse(cgiIt, poll_sets, clients, errorChunk);
			Logger::debug("Sent timeout error chunk for FD " + Utils::toString(cgiIt->clientFd));
		}
		// Close connection
		for (std::vector<ClientHandler>::iterator clientIt = clients.begin(); clientIt != clients.end(); ++clientIt) {
			if (clientIt->getFd() == cgiIt->clientFd) {
				clientIt->setResponse(""); // Clear any pending response
				break;
			}
		}
		removeCGIFDsFromPollSets(poll_sets, fdsToClose);
		closeCGIFDs(fdsToClose, cgiIt);
		cgiIt->isActive = false;
		Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
		// Close client connection immediately after sending error chunk
		for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end(); ++it) {
			if (it->fd == cgiIt->clientFd) {
				close(it->fd);
				poll_sets.erase(it);
				for (std::vector<ClientHandler>::iterator clIt = clients.begin(); clIt != clients.end(); ++clIt) {
					if (clIt->getFd() == cgiIt->clientFd) {
						clients.erase(clIt);
						break;
					}
				}
				Logger::info("Closed client FD " + Utils::toString(cgiIt->clientFd) + " after timeout");
				break;
			}
		}
		return;
		// REMOVED: Previous timeout handling with generateServerErrorResponse
		// std::string errorChunk = generateServerErrorResponse(cgiIt, 503, "Service Unavailable");
		// setClientResponse(cgiIt, poll_sets, clients, errorChunk);
		// removeCGIFDsFromPollSets(poll_sets, fdsToClose);
		// closeCGIFDs(fdsToClose, cgiIt);
		// cgiIt->isActive = false;
		// Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
	}

	//Handle timeout for CGI
	// if (cgiIt->cgi->isTimedOut())
	// {
	//     // Logger::warn("CGI FD " + Utils::toString(fd) + " timed out and killed");
	//     // std::string errorChunk = generateServerErrorResponse(cgiIt, 503, "Service Unavailable");
	//     // setClientResponse(cgiIt, poll_sets, clients, errorChunk);
	//     // removeCGIFDsFromPollSets(poll_sets, fdsToClose);
	//     // closeCGIFDs(fdsToClose, cgiIt);
	//     // cgiIt->isActive = false;
	//     // Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
	//     // return;
	//     Logger::warn("CGI FD " + Utils::toString(fd) + " timed out and killed");
	//     std::string finalChunk;
	//     // If chunked encoding was active, send a final chunk to terminate the stream
	//     if (cgiIt->isChunked && cgiIt->firstResponseSent) {
	//         finalChunk = formatChunkedData("", true); // Send 0\r\n\r\n
	//         setClientResponse(cgiIt, poll_sets, clients, finalChunk);
	//         Logger::debug("Sent final chunk to terminate chunked stream for FD " + Utils::toString(cgiIt->clientFd));
	//     }
	//     // Generate and send 503 response
	//     std::string errorChunk = generateServerErrorResponse(cgiIt, 503, "Service Unavailable");
	//     setClientResponse(cgiIt, poll_sets, clients, errorChunk);
	//     removeCGIFDsFromPollSets(poll_sets, fdsToClose);
	//     closeCGIFDs(fdsToClose, cgiIt);
	//     cgiIt->isActive = false;
	//     // Ensure connection is closed
	//     for (std::vector<ClientHandler>::iterator clientIt = clients.begin(); clientIt != clients.end(); ++clientIt) {
	//         if (clientIt->getFd() == cgiIt->clientFd) {
	//             clientIt->setResponse(""); // Clear any pending response
	//             break;
	//         }
	//     }
	//     Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
	//     return;
	// }

	// Handle stderr separately
	if (fd == cgiIt->cgi->getErrPipeReadFd())
	{
		char buffer[1024];
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes > 0)
		{
			buffer[bytes] = '\0';
			Logger::error("CGI stderr for Server " + cgiIt->serverConfig->getIP() + ":" +
						cgiIt->serverConfig->getPort() + ", clientFd=" +
						Utils::toString(cgiIt->clientFd) + ": " + std::string(buffer));
		}
		else if (bytes == 0 || bytes == -1)
		{
			Logger::debug("EOF or error on CGI error FD " + Utils::toString(fd));
			cgiIt->cgi->closePipe(fdsToClose[2]);
			fdsToClose[2] = -1;
		}
		return;
	}

	// Read data from CGI stdout
	if (fd == cgiIt->cgi->getOutPipeReadFd())
	{
		char buffer[1024];
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes > 0)
		{
			buffer[bytes] = '\0';
			std::string rawData = std::string(buffer);
			Logger::debug("Read " + Utils::toString(bytes) + " bytes from CGI FD " + Utils::toString(fd) +
						" for server " + cgiIt->serverConfig->getIP() + ":" + cgiIt->serverConfig->getPort() +
						", raw: [" + rawData + "]");

			cgiIt->cgi->appendOutput(fd, buffer, bytes);
			std::string newData = rawData;
			std::string chunk = generateCGIResponse(cgiIt, newData, false);

			// setClientResponse(cgiIt, poll_sets, clients, chunk);
			if (!chunk.empty())
			{
				// Retry write up to 3 times to handle transient issues
				int retries = 3;
				while (retries > 0)
				{
					setClientResponse(cgiIt, poll_sets, clients, chunk);
					// Check if write succeeded (requires ClientHandler to report write status, assumed here)
					// If ClientHandler doesn't expose write status, rely on logs
					Logger::debug("Attempted to write " + Utils::toString(chunk.size()) + " bytes to client FD " +
								Utils::toString(cgiIt->clientFd));
					break; // Assume success for now; refine with ClientHandler if needed
					retries--;
					if (retries > 0) {
						Logger::debug("Retrying write to client FD " + Utils::toString(cgiIt->clientFd) +
									", retries left: " + Utils::toString(retries));
						usleep(1000);// Brief delay to allow socket to clear
					}
				}
				if (retries == 0)
				{
					Logger::warn("Failed to write to client FD " + Utils::toString(cgiIt->clientFd) +
								" after 3 retries");
				}
			}
			cgiIt->firstResponseSent = true;
		}
		else if (bytes == 0)
		{
			Logger::debug("EOF on CGI FD " + Utils::toString(fd));
			std::string finalChunk = generateCGIResponse(cgiIt, "", true);

			if (!finalChunk.empty())  //setClientResponse(cgiIt, poll_sets, clients, finalChunk);
			{
				setClientResponse(cgiIt, poll_sets, clients, finalChunk);
				Logger::debug("Sent final CGI response of " + Utils::toString(finalChunk.size()) +
							" bytes to client FD " + Utils::toString(cgiIt->clientFd));
			}
			cgiIt->cgi->setOutputDone();
			Logger::debug("Before cleanup: in=" + Utils::toString(fdsToClose[0]) +
						", out=" + Utils::toString(fdsToClose[1]) +
						", err=" + Utils::toString(fdsToClose[2]));
			removeCGIFDsFromPollSets(poll_sets, fdsToClose);
			closeCGIFDs(fdsToClose, cgiIt);
			cgiIt->isActive = false;
			Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
		}
		else
		{
			Logger::warn("Read error on CGI output FD " + Utils::toString(fd) + " for clientFd=" + Utils::toString(cgiIt->clientFd));
			// Enhanced error handling for read failures
			std::string errorReason;

			// Check for specific conditions without errno
			struct pollfd checkPoll;
			checkPoll.fd = fd;
			checkPoll.events = POLLIN;
			int pollResult = poll(&checkPoll, 1, 0);

			if (pollResult < 0) errorReason = "polling failed";
			else if (checkPoll.revents & POLLNVAL) errorReason = "invalid file descriptor";
			else if (checkPoll.revents & POLLERR) errorReason = "error condition";
			else if (checkPoll.revents & POLLHUP)
			{
				errorReason = "hung up";
				Logger::debug("POLLHUP on CGI FD " + Utils::toString(fd) + ", checking buffered output");
				std::string finalChunk = generateCGIResponse(cgiIt, "", true);
				if (!finalChunk.empty()) {
					setClientResponse(cgiIt, poll_sets, clients, finalChunk);
					Logger::debug("Sent final CGI response of " + Utils::toString(finalChunk.size()) +
								" bytes to client FD " + Utils::toString(cgiIt->clientFd));
				}
				cgiIt->cgi->setOutputDone();
				removeCGIFDsFromPollSets(poll_sets, fdsToClose);
				closeCGIFDs(fdsToClose, cgiIt);
				cgiIt->isActive = false;
				Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
				return;
			}
			else errorReason = "unknown error";

			Logger::warn("Read error on CGI output FD " + Utils::toString(fd) +
						" for clientFd=" + Utils::toString(cgiIt->clientFd) +
						": " + errorReason);
			//handleCGIError(fd, poll_sets, clients);
			Logger::warn("Read error on CGI output FD " + Utils::toString(fd) +
						" for clientFd=" + Utils::toString(cgiIt->clientFd) + ": " + errorReason);
			std::string errorChunk = generateServerErrorResponse(cgiIt, 500, "Internal Server Error");
			setClientResponse(cgiIt, poll_sets, clients, errorChunk);
			removeCGIFDsFromPollSets(poll_sets, fdsToClose);
			closeCGIFDs(fdsToClose, cgiIt);
			cgiIt->isActive = false;
			Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
		}
	}
}

std::vector<struct pollfd>::iterator CGIHandler::findPollIterator(int fd,
	std::vector<struct pollfd>& poll_sets)
{
	std::vector<struct pollfd>::iterator pollIt = poll_sets.begin();
	while (pollIt != poll_sets.end() && pollIt->fd != fd)
		++pollIt;
	return pollIt;
}

void CGIHandler::initializeFDsToClose(std::vector<CGITracker>::iterator cgiIt, int fdsToClose[3])
{
	fdsToClose[0] = cgiIt->cgi->getInPipeWriteFd();
	fdsToClose[1] = cgiIt->cgi->getOutPipeReadFd();
	fdsToClose[2] = cgiIt->cgi->getErrPipeReadFd();
}

void CGIHandler::handlePositiveBytes(int fd, char* buffer, ssize_t bytes, std::vector<CGITracker>::iterator cgiIt)
{
	cgiIt->cgi->appendOutput(fd, buffer, bytes);
	Logger::debug("Read " + Utils::toString(bytes) + " bytes from CGI output FD " + \
					Utils::toString(fd) + " for Server " + cgiIt->serverConfig->getIP() + \
					":" + cgiIt->serverConfig->getPort() + ", clientFd=" + Utils::toString(cgiIt->clientFd));
}

void CGIHandler::handleEOF(std::vector<CGITracker>::iterator cgiIt,
	std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, int fdsToClose[3])
{
	Logger::debug("EOF on CGI");
	cgiIt->cgi->setOutputDone();
	std::string finalChunk = generateCGIResponse(cgiIt, "", true);
	setClientResponse(cgiIt, poll_sets, clients, finalChunk);
	removeCGIFDsFromPollSets(poll_sets, fdsToClose);
	closeCGIFDs(fdsToClose, cgiIt);
	cgiIt->isActive = false;
	removeCGI(cgiIt);
	Logger::debug("CGI cleanup completed for clientFd=" + Utils::toString(cgiIt->clientFd));
}

std::string CGIHandler::generateCGIResponse(std::vector<CGITracker>::iterator cgiIt,
						const std::string& newData, bool final)
{
	// Extract script type and paths
	std::string scriptPath = cgiIt->cgi->getScriptPath();
	bool isUploadPy = isUploadScript(scriptPath);
	bool isLoopPy = isLoopScript(scriptPath);
	bool isFortunePhp = isFortuneScript(scriptPath);
	Logger::debug("Generating CGI response for script: " + scriptPath);

	// Handle timeout scenarios
	if (cgiIt->cgi->isTimedOut())
	{
		handleTimeout(cgiIt, isUploadPy, isLoopPy);
		final = true;
	}

	// Special handling for loop.py streaming
	if (isLoopPy) { return handleLoopPyResponse(cgiIt, newData, final); }

	// Buffer new data
	if (!newData.empty())
	{
		cgiIt->bufferedOutput += newData;
		// Parse headers if not already done
		if (!cgiIt->headersParsed) { parseHeaders(cgiIt, newData); }
	}

	bool isComplete = final || cgiIt->cgi->isOutputDone();
	Logger::debug("isComplete: " + Utils::toString(isComplete) + ", final: " + Utils::toString(final));

	// Handle abnormal termination without headers
	if (isComplete && !cgiIt->headersParsed && cgiIt->statusCode != 504) { setErrorState(cgiIt, isUploadPy, isLoopPy); }

	// Handle fortune.php responses
	if (isFortunePhp) { return handleFortuneResponse(cgiIt, isComplete); }

	// Handle upload.py responses
	if (isUploadPy) { return handleUploadResponse(cgiIt, isComplete); }

	// Handle other scripts with either chunked or full responses
	return handleGenericResponse(cgiIt, isComplete, final);
}

// Helpers of generateCGIResponse
void CGIHandler::handleTimeout(std::vector<CGITracker>::iterator cgiIt, bool isUploadPy, bool isLoopPy)
{
	cgiIt->statusCode = 503;
	cgiIt->headersParsed = true;

	if (isUploadPy)
	{
		cgiIt->bufferedOutput = "{\"error\": \"Service Unavailable\", \"code\": 503}";
		cgiIt->contentType = "application/json";
	}
	else if (isLoopPy)
	{
		cgiIt->bufferedOutput = "503 Service Unavailable";
		cgiIt->contentType = "text/plain";
	}
	else
	{
		cgiIt->bufferedOutput = HttpException(503, "Service Unavailable").getBody();
		cgiIt->contentType = "text/html";
	}
	Logger::warn("CGI timed out, setting status 503");
}

std::string CGIHandler::handleLoopPyResponse(std::vector<CGITracker>::iterator cgiIt,
	const std::string& newData, bool final)
{
	// Initial chunked response setup
	if (!cgiIt->firstResponseSent)
	{
		HttpResponse response = createStandardResponse(200, "", "text/plain", false);
		response.setHeader("Transfer-Encoding", "chunked");
		cgiIt->isChunked = true;
		cgiIt->firstResponseSent = true;
		Logger::debug("Sent initial chunked headers for loop.py on FD " + Utils::toString(cgiIt->clientFd));
		return response.composeResponse();
	}

	// Handle timeout - new
	if (cgiIt->cgi->isTimedOut())
	{
		cgiIt->statusCode = 503;
		std::string errorMessage = "503 Service Unavailable";
		std::string formattedChunk = formatChunkedData(errorMessage, true); // Final chunk
		Logger::debug("Sending final error chunk for loop.py timeout, size: " + Utils::toString(errorMessage.length()) +
					" on FD " + Utils::toString(cgiIt->clientFd));
		cgiIt->bufferedOutput.clear();
		return formattedChunk;
	}

	// Stream data in chunks
	if (!final && !newData.empty())
	{
		std::string formattedChunk = formatChunkedData(newData, false);
		Logger::debug("Sending chunk for loop.py, size: " + Utils::toString(newData.length()) +
					", chunk length: " + Utils::toString(formattedChunk.length()) +
					" on FD " + Utils::toString(cgiIt->clientFd));
		return formattedChunk;
	}

	// Handle final response or error
	if (final)
	{
		if (cgiIt->statusCode != 200)
		{
			std::ostringstream chunk;
			chunk << std::hex << cgiIt->bufferedOutput.length() << "\r\n" << cgiIt->bufferedOutput << "\r\n0\r\n\r\n";
			std::string finalResponse = formatChunkedData(cgiIt->bufferedOutput, true);
			Logger::error("503 - Service Unavailable Due to Timeout, status " + Utils::toString(cgiIt->statusCode) +
						" on FD " + Utils::toString(cgiIt->clientFd));
			cgiIt->bufferedOutput.clear();
			return finalResponse;
		}
		Logger::debug("Sending final chunk for loop.py on FD " + Utils::toString(cgiIt->clientFd));
		return formatChunkedData("", true);
	}
	// if (final)
	// {
	//     Logger::debug("Sending final chunk for loop.py on FD " + Utils::toString(cgiIt->clientFd));
	//     return formatChunkedData("", true);
	// }
	return "";
}

void CGIHandler::parseHeaders(std::vector<CGITracker>::iterator cgiIt, const std::string& newData)
{
	size_t bodyStart = newData.find("\r\n\r\n");
	if (bodyStart == std::string::npos)
		bodyStart = newData.find("\n\n");

	if (bodyStart != std::string::npos)
	{
		std::string headers = newData.substr(0, bodyStart);
		std::string body = newData.substr(bodyStart + (newData[bodyStart] == '\r' ? 4 : 2));
		Logger::debug("Headers section: " + headers);

		// Extract and process headers
		std::string::size_type pos = 0;
		while (pos < headers.length())
		{
			std::string::size_type end = headers.find("\r\n", pos);
			if (end == std::string::npos) end = headers.find("\n", pos);
			if (end == std::string::npos) end = headers.length();
			std::string line = headers.substr(pos, end - pos);
			if (line.empty()) break;

			Logger::debug("Processing header: " + line);
			std::string::size_type colonPos = line.find(':');
			if (colonPos != std::string::npos)
			{
				std::string key = line.substr(0, colonPos);
				std::string value = line.substr(colonPos + 1);
				while (!value.empty() && std::isspace(value[0])) value = value.substr(1);
				while (!value.empty() && std::isspace(value[value.length() - 1])) value = value.substr(0, value.length() - 1);

				// Case-insensitive check for "Status" header (just in case)
				std::string lowerKey = key;
				for (std::string::iterator it = lowerKey.begin(); it != lowerKey.end(); ++it)
					*it = std::tolower(static_cast<unsigned char>(*it));

				if (key == "Status" || lowerKey == "status")
				{
					parseStatusHeader(cgiIt, value);
					if (cgiIt->statusCode == 418)
					{
						Logger::debug("418 Teapot status detected - storing custom body content: " +
									Utils::toString(body.length()) + " bytes");
					}
				}
				else if (key == "Content-Type" || key == "Content-type")
				{
					cgiIt->contentType = value;
					Logger::debug("Content-Type set to: " + cgiIt->contentType);
				}
				else
				{
					cgiIt->cgi_headers[key] = value;
					Logger::debug("Stored header: " + key + " = " + value);
				}
			}
			pos = end + (headers[end] == '\r' ? 2 : 1);
		}
		cgiIt->headersParsed = true;
		cgiIt->bufferedOutput = body;
		Logger::debug("Headers parsed, bufferedOutput size: " + Utils::toString(body.length()) + " bytes");
	}
}

void CGIHandler::parseStatusHeader(std::vector<CGITracker>::iterator cgiIt, const std::string& value)
{
	size_t spacePos = value.find(' ');
	if (spacePos != std::string::npos)
	{
		std::string codeStr = value.substr(0, spacePos);
		int code = Utils::toInt(codeStr);
		if (code >= 100 && code <= 599)
		{
			cgiIt->statusCode = code;
			// Enhanced error logging with specific messages for all required status codes
			switch (code)
			{
				case 400:
					Logger::error("400 Bad Request from CGI");
					break;
				case 403:
					Logger::error("403 Forbidden from CGI");
					break;
				case 404:
					Logger::error("404 Not Found from CGI");
					break;
				case 409:
					Logger::error("409 Conflict from CGI");
					break;
				case 413:
					Logger::error("413 Payload Too Large from CGI");
					break;
				case 415:
					Logger::error("415 Unsupported Media Type from CGI");
					break;
				case 418: // Easter egg
					Logger::info("418 I'm a Teapot from CGI");
					break;
				case 500:
					Logger::error("500 Internal Server Error from CGI");
					break;
				case 501:
					Logger::error("501 Not Implemented from CGI");
					break;
				case 502:
					Logger::error("502 Bad Gateway from CGI");
					break;
				case 503:
					Logger::error("503 Service Unavailable from CGI");
					break;
				case 504:
					Logger::error("504 Gateway Timeout from CGI");
					break;
				case 505:
					Logger::error("505 HTTP Version Not Supported from CGI");
					break;
				default:
					if (code >= 400) Logger::error("HTTP status " + codeStr);
					else Logger::info("CGI Status: " + value);
					break;
			}
		}
		else
		{
			Logger::warn("Invalid CGI status code: " + codeStr + ", defaulting to 500");
			cgiIt->statusCode = 500;
		}
	}
	else
	{
		Logger::warn("Malformed Status header: " + value + ", defaulting to 500");
		cgiIt->statusCode = 500;
	}

	Logger::debug("Parsed Status: " + value);
}

void CGIHandler::setErrorState(std::vector<CGITracker>::iterator cgiIt, bool isUploadPy, bool isLoopPy)
{
	int code = cgiIt->statusCode;
	if (code < 400) { code = 500; } // Default to Internal Server Error if no error code set

	cgiIt->statusCode = code;
	cgiIt->headersParsed = true;

	std::string errorMsg;
	switch (code)
	{
		case 400:
			errorMsg = "Bad Request";
			break;
		case 403:
			errorMsg = "Forbidden";
			break;
		case 404:
			errorMsg = "Not Found";
			break;
		case 409:
			errorMsg = "Conflict";
			break;
		case 413:
			errorMsg = "Payload Too Large";
			break;
		case 415:
			errorMsg = "Unsupported Media Type";
			break;
		case 500:
			errorMsg = "Internal Server Error";
			break;
		case 501:
			errorMsg = "Not Implemented";
			break;
		case 502:
			errorMsg = "Bad Gateway";
			break;
		case 503:
			errorMsg = "Service Unavailable: Took Too Long!";
			break;
		case 504:
			errorMsg = "Gateway Timeout";
			break;
		case 505:
			errorMsg = "HTTP Version Not Supported";
			break;
		default:
			errorMsg = "Internal Server Error";
			code = 500;
			break;
	}

	// Format the response based on script type
	if (isUploadPy)
	{
		cgiIt->bufferedOutput = "{\"error\": \"" + errorMsg + "\", \"code\": " + Utils::toString(code) + "}";
		cgiIt->contentType = "application/json";
	}
	else if (isLoopPy)
	{
		cgiIt->bufferedOutput = Utils::toString(code) + " " + errorMsg;
		cgiIt->contentType = "text/plain";
	}
	else
	{
		cgiIt->bufferedOutput = HttpException(code, errorMsg).getBody();
		cgiIt->contentType = "text/html";
	}
	Logger::warn(Utils::toString(code) + " " + errorMsg + " with Content-Type: " + cgiIt->contentType);
}

std::string CGIHandler::handleFortuneResponse(std::vector<CGITracker>::iterator cgiIt, bool isComplete)
{
	if (cgiIt->statusCode == 418 && !isComplete) // Easter egg case
	{
		Logger::debug("418 Teapot detected, buffering content until complete. Current size: " +
					Utils::toString(cgiIt->bufferedOutput.length()) + " bytes");
		return ""; // Return empty string to avoid sending partial responses
	}

	if (cgiIt->statusCode >= 400) // Error cases
	{
		Logger::warn("Fortune.php returning status code: " + Utils::toString(cgiIt->statusCode));

		//create and return error response
		std::string responseBody = cgiIt->bufferedOutput;
		HttpResponse response = createStandardResponse(cgiIt->statusCode, responseBody, cgiIt->contentType.empty() ? "text/html" : cgiIt->contentType, true);

		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			response.setHeader(it->first, it->second);
		}

		std::string fullResponse = response.composeResponse();
		correctStatusLine(cgiIt, fullResponse);

		cgiIt->firstResponseSent = true;
		cgiIt->bufferedOutput.clear();
		return fullResponse;
	}

	if (isComplete) // other cases
	{
		std::string responseBody = cgiIt->bufferedOutput;
		HttpResponse response = createStandardResponse(cgiIt->statusCode, responseBody, cgiIt->contentType.empty() ? "text/html" : cgiIt->contentType, true);
		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			response.setHeader(it->first, it->second);
		}

		std::string fullResponse = response.composeResponse();
		correctStatusLine(cgiIt, fullResponse);

		cgiIt->firstResponseSent = true;
		cgiIt->bufferedOutput.clear();
		return fullResponse;
	}

	Logger::debug("Buffering fortune.php output, buffered size: " +
				Utils::toString(cgiIt->bufferedOutput.length()));
	return "";
}

void CGIHandler::correctStatusLine(std::vector<CGITracker>::iterator cgiIt, std::string& fullResponse)
{
	if (cgiIt->statusCode == 505 && cgiIt->cgi_headers.count("Location"))
	{
		std::string expectedStatusLine = "HTTP/1.1 " + Utils::toString(cgiIt->statusCode) +
										" HTTP Version Not Supported";
		if (fullResponse.find(expectedStatusLine) != 0)
		{
			size_t endOfStatusLine = fullResponse.find("\r\n");
			if (endOfStatusLine != std::string::npos)
			{
				fullResponse.replace(0, endOfStatusLine, expectedStatusLine);
				Logger::debug("Corrected status line to: " + expectedStatusLine);
			}
		}
	}
}

std::string CGIHandler::handleUploadResponse(std::vector<CGITracker>::iterator cgiIt, bool isComplete)
{
	if (isComplete || cgiIt->statusCode != 200)
	{
		std::string responseBody = cgiIt->bufferedOutput;
		HttpResponse response = createStandardResponse(cgiIt->statusCode, responseBody,
			cgiIt->contentType.empty() ? "application/json" : cgiIt->contentType, true);

		// Add headers EXCEPT Transfer-Encoding for upload.py
		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			if (it->first != "Transfer-Encoding")
				response.setHeader(it->first, it->second);
		}

		std::string fullResponse = response.composeResponse();
		Logger::debug("Sending upload.py response, length: " + Utils::toString(fullResponse.length()));
		Logger::debug("Response start: " + fullResponse.substr(0, 100));
		cgiIt->firstResponseSent = true;
		cgiIt->bufferedOutput.clear();
		return fullResponse;
	}
	else
	{
		Logger::debug("Buffering upload.py output, buffered size: " +
					Utils::toString(cgiIt->bufferedOutput.length()));
		return "";
	}
}

std::string CGIHandler::handleGenericResponse(std::vector<CGITracker>::iterator cgiIt,
									bool isComplete, bool final)
{
	(void)final;
	std::string body = cgiIt->bufferedOutput;

	if (isComplete || cgiIt->statusCode != 200)
	{
		// Complete response - use Content-Length
		std::string responseBody = cgiIt->bufferedOutput;

		HttpResponse response = createStandardResponse(cgiIt->statusCode, responseBody, cgiIt->contentType.empty() ? "text/html" : cgiIt->contentType, true);

		// Add all headers EXCEPT Transfer-Encoding
		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			if (it->first != "Transfer-Encoding")
			{
				response.setHeader(it->first, it->second);
				Logger::debug("Added header: " + it->first + " = " + it->second);
			}
		}

		std::string fullResponse = response.composeResponse();
		Logger::debug("Sending full response, length: " + Utils::toString(fullResponse.length()));
		Logger::debug("Response start: " + fullResponse.substr(0, 100));
		cgiIt->firstResponseSent = true;
		cgiIt->bufferedOutput.clear();
		return fullResponse;
	}
	else if (!isComplete && cgiIt->statusCode == 200 && cgiIt->bufferedOutput.length() < 4096 && !cgiIt->isChunked)
	{
		// Start chunked encoding for other scripts
		HttpResponse response(cgiIt->statusCode, "");
		response.setHeader("Content-Type", cgiIt->contentType);
		response.setHeader("Transfer-Encoding", "chunked");
		response.setHeader("Cache-Control", "no-cache");
		response.setHeader("Connection", "keep-alive");
		response.setHeader("Date", response.findTimeStamp());

		// Add all headers EXCEPT Content-Length
		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			if (it->first != "Content-Length")
				response.setHeader(it->first, it->second);
		}

		std::string initialResponse = response.composeResponse();
		std::ostringstream chunk;
		chunk << std::hex << body.length() << "\r\n" << body << "\r\n";
		std::string fullResponse = initialResponse + chunk.str();
		Logger::debug("Sending chunked response: " + fullResponse);
		cgiIt->firstResponseSent = true;
		cgiIt->isChunked = true;
		return fullResponse;
	}
	else if (cgiIt->isChunked)
	{
		// Continue with chunked encoding
		if (!isComplete)
		{
			std::string formattedChunk = formatChunkedData(body, false);
			Logger::debug("Sending chunk: " + formattedChunk.substr(0, 50) + "...");
			return formattedChunk;
		}
		else
		{
			std::string finalChunk = formatChunkedData("", true);
			Logger::debug("Sending final chunk: " + finalChunk);
			return finalChunk;
		}
	}
	Logger::debug("Buffering, buffered size: " + Utils::toString(cgiIt->bufferedOutput.length()));
	return "";
}

// Function to generate server timeout response
std::string CGIHandler::generateServerTimeoutResponse(std::vector<CGITracker>::iterator cgiIt)
{
	std::string scriptPath = cgiIt->cgi->getScriptPath();
	bool isUploadPy = isUploadScript(scriptPath);
	bool isLoopPy = isLoopScript(scriptPath);
	// bool isFortunePhp = isFortuneScript(scriptPath);  // Uncomment if needed

	cgiIt->statusCode = 503;
	cgiIt->headersParsed = true;

	// Set error response based on script type
	if (isUploadPy)
	{
		cgiIt->bufferedOutput = "{\"error\": \"Service Unavailable\", \"code\": 503}";
		cgiIt->contentType = "application/json";
	}
	else if (isLoopPy)
	{
		cgiIt->bufferedOutput = "503 Service Unavailable";
		cgiIt->contentType = "text/plain";
	}
	else // Us HttpException
	{
		cgiIt->bufferedOutput = HttpException(503, "Service Unavailable").getBody();
		cgiIt->contentType = "text/html";
	}

	std::string fullResponse;

	// Prepend final chunk if chunked encoding was active
	if (cgiIt->isChunked)
	{
		fullResponse = formatChunkedData(cgiIt->bufferedOutput, true);
		Logger::debug("Sent chunked 503 response with final chunk, length: " + Utils::toString(fullResponse.length()));
	}
	// // For chunked responses, send proper termination if needed - to recheck
	// if (cgiIt->isChunked && cgiIt->firstResponseSent)
	// {
	//     // CHANGE: Use formatChunkedData helper
	//     std::string finalChunk = formatChunkedData("", true);
	//     write(cgiIt->clientFd, finalChunk.c_str(), finalChunk.length());
	//     Logger::debug("Sent final chunk to terminate response after CTRL+C");
	// }
	else
	{
		HttpResponse response = createStandardResponse(cgiIt->statusCode, cgiIt->bufferedOutput, cgiIt->contentType, true);
		for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
			it != cgiIt->cgi_headers.end(); ++it)
		{
			if (it->first != "Transfer-Encoding") // Don't mix with chunked encoding
				response.setHeader(it->first, it->second);
		}
		fullResponse = response.composeResponse();
		Logger::debug("Sent non-chunked 503 response, length: " + Utils::toString(fullResponse.length()));
	}
	Logger::error("503 Service Unavailable sent for script " + scriptPath + " due to timeout or interruption");
	cgiIt->firstResponseSent = true;
	cgiIt->bufferedOutput.clear();
	return fullResponse;
}

void CGIHandler::setClientResponse(std::vector<CGITracker>::iterator cgiIt,
	std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, const std::string& cgiResponse)
{
	bool clientFound = false;
	std::vector<ClientHandler>::iterator clientIt = clients.begin();
	for (; clientIt != clients.end(); ++clientIt)
	{
		if (clientIt->getFd() == cgiIt->clientFd)
		{
			std::string currentResponse = clientIt->getResponse();
			std::string newResponse = currentResponse.empty() ? cgiResponse : currentResponse + cgiResponse;
			clientIt->setResponse(newResponse);
			ssize_t bytesWritten = write(clientIt->getFd(), newResponse.c_str(), newResponse.length());
			if (bytesWritten < 0)
			{
				// Detect broken pipe without using errno
				Logger::error("Write failed to client FD " + Utils::toString(clientIt->getFd()) +
							" - likely broken pipe (client disconnected)");

				// Check if FD is already invalid (can detect this without errno)
				struct pollfd testPoll;
				testPoll.fd = clientIt->getFd();
				testPoll.events = POLLIN;
				if (poll(&testPoll, 1, 0) < 0 || (testPoll.revents & POLLNVAL))
				{
					Logger::error("Confirmed client FD " + Utils::toString(clientIt->getFd()) +
								" is invalid - client disconnected during CGI processing");
				}
			}
			else if (bytesWritten == 0)
				Logger::warn("Wrote 0 bytes to FD " + Utils::toString(clientIt->getFd()) + ", response length: " + Utils::toString(newResponse.length()));
			else
			{
				Logger::debug("Wrote " + Utils::toString(bytesWritten) + " bytes to FD " + Utils::toString(clientIt->getFd()));
				clientIt->setResponse("");
			}
			clientFound = true;
			Logger::info("Response appended and flushed for client FD " + Utils::toString(cgiIt->clientFd) +
						" on Server " + cgiIt->serverConfig->getIP() + ":" + cgiIt->serverConfig->getPort());
			break;
		}
	}
	if (clientFound)
	{
		std::vector<struct pollfd>::iterator pit = poll_sets.begin();
		for (; pit != poll_sets.end(); ++pit)
		{
			if (pit->fd == cgiIt->clientFd)
			{
				pit->events = POLLOUT;
				Logger::info("Client FD " + Utils::toString(cgiIt->clientFd) + " set to POLLOUT");
				break;
			}
		}
	}
}

void CGIHandler::closeCGIFDs(int fdsToClose[3], std::vector<CGITracker>::iterator cgiIt)
{
	if (!cgiIt->cgi)
	{
		Logger::debug("No CGI object found when closing FDs, skipping");
		return;
	}

	for (int i = 0; i < 3; ++i)
	{
		if (fdsToClose[i] > 0)
		{
			Logger::debug("Closing FD " + Utils::toString(fdsToClose[i]));
			cgiIt->cgi->closePipe(fdsToClose[i]);
			// Make sure CGI object knows this FD is closed here?
			fdsToClose[i] = -1;
		}
	}
}

void CGIHandler::removeCGIFDsFromPollSets(std::vector<struct pollfd>& poll_sets, int fdsToClose[3])
{
	Logger::debug("Starting FD removal: in=" + Utils::toString(fdsToClose[0]) +
				", out=" + Utils::toString(fdsToClose[1]) +
				", err=" + Utils::toString(fdsToClose[2]));
	std::vector<struct pollfd>::iterator tempIt = poll_sets.begin();
	while (tempIt != poll_sets.end())
	{
		bool found = false;
		for (int j = 0; j < 3; ++j)
		{
			if (fdsToClose[j] != -1 && tempIt->fd == fdsToClose[j])
			{
				Logger::debug("Removing FD " + Utils::toString(tempIt->fd) + " from poll_sets");
				tempIt = poll_sets.erase(tempIt);
				found = true;
				break;
			}
		}
		if (!found)
			++tempIt;
	}
	Logger::debug("Finished removing CGI FDs from poll_sets, remaining: " + Utils::toString(poll_sets.size()));
}

void CGIHandler::handleCGIError(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
	std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
	if (cgiIt == _cgiQueue.end())
	{
		Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
		return;
	}

	std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);

	// Handle error pipe
	if (fd == cgiIt->cgi->getErrPipeReadFd())
	{
		char buffer[1024];
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes > 0)
		{
			buffer[bytes] = '\0';
			Logger::error("CGI stderr for Server " + cgiIt->serverConfig->getIP() + ":" +  \
						cgiIt->serverConfig->getPort() + ", clientFd=" + \
						Utils::toString(cgiIt->clientFd) + ": " + std::string(buffer));
		}
		cgiIt->cgi->closePipe(fd);
		if (pollIt != poll_sets.end())
			poll_sets.erase(pollIt);
		return; // Keep CGI alive to read more stderr
	}
	// Check for SIGINT-related errors
	bool isSigInt = false;
	char buffer[1024];
	ssize_t bytes = read(cgiIt->cgi->getErrPipeReadFd(), buffer, sizeof(buffer) - 1);
	if (bytes > 0)
	{
		buffer[bytes] = '\0';
		std::string stderrOutput = std::string(buffer);
		if (stderrOutput.find("KeyboardInterrupt") != std::string::npos) {
			isSigInt = true;
			Logger::error("CGI process interrupted (SIGINT) for clientFd=" + Utils::toString(cgiIt->clientFd));

			// Ensure CGI process is properly killed
			if (cgiIt->cgi && cgiIt->cgi->getProcessId() > 0)
			{
				kill(cgiIt->cgi->getProcessId(), SIGKILL);
				Logger::info("Killed CGI process " + Utils::toString(cgiIt->cgi->getProcessId()) +
						" due to client cancellation (CTRL+C)");
			}

			// For chunked responses, send proper termination if needed
			if (cgiIt->isChunked && cgiIt->firstResponseSent)
			{
				// std::string finalChunk = "0\r\n\r\n";
				// write(cgiIt->clientFd, finalChunk.c_str(), finalChunk.length());
				// Logger::debug("Sent final chunk to terminate response after CTRL+C");
				std::string finalChunk = formatChunkedData("", true);
				write(cgiIt->clientFd, finalChunk.c_str(), finalChunk.length());
				Logger::debug("Sent final chunk to terminate response after CTRL+C");
			}
		}
	}

	// Set error response for client
	bool clientFound = false;
	std::vector<ClientHandler>::iterator clientIt = clients.begin();
	for (; clientIt != clients.end(); ++clientIt)
	{
		if (clientIt->getFd() == cgiIt->clientFd)
		{
			if (clientIt->getResponse().empty())
			{
				HttpResponse response;
				if (isSigInt)
					clientIt->setResponse(createStandardResponse(200, "Request cancelled by user", "text/plain", true).composeResponse());
				else
					clientIt->setResponse(createStandardResponse(500, "CGI execution failed", "text/plain", true).composeResponse());

				response.setHeader("Content-Length", Utils::toString(response.getBody().length()));
				response.setHeader("Cache-Control", "no-cache");
				response.setHeader("Connection", "close");
				response.setHeader("Date", response.findTimeStamp());
				clientIt->setResponse(response.composeResponse());
			}
			clientFound = true;
			break;
		}
	}

	// Set client to POLLOUT to send error response
	if (clientFound)
	{
		std::vector<struct pollfd>::iterator pit = poll_sets.begin();
		for (; pit != poll_sets.end(); ++pit)
		{
			if (pit->fd == cgiIt->clientFd)
			{
				pit->events = POLLOUT;
				break;
			}
		}
	}

	// Clean up CGI resources
	int fdsToClose[3];
	initializeFDsToClose(cgiIt, fdsToClose);
	removeCGIFDsFromPollSets(poll_sets, fdsToClose);
	closeCGIFDs(fdsToClose, cgiIt);
	removeCGI(cgiIt);
	Logger::error("CGI error handled for server " + cgiIt->serverConfig->getIP() + ":" +\
				cgiIt->serverConfig->getPort() + ", clientFd=" + Utils::toString(cgiIt->clientFd) + \
				" on FD " + Utils::toString(fd));
}

std::vector<CGITracker>::iterator CGIHandler::findCGI(int fd)
{
	std::vector<CGITracker>::iterator it;
	for (it = _cgiQueue.begin(); it != _cgiQueue.end(); ++it)
	{
		if (it->pipeFd == fd || it->cgi->getInPipeWriteFd() == fd ||
			it->cgi->getOutPipeReadFd() == fd || it->cgi->getErrPipeReadFd() == fd ||
			it->clientFd == fd)
		{
			return it;
		}
	}
	return _cgiQueue.end();
}

void CGIHandler::removeCGI(std::vector<CGITracker>::iterator it)
{
	if (it->cgi)
	{
		delete it->cgi;
		it->cgi = NULL;
	}
	it->isActive = false; // Explicitly mark as inactive
	_cgiQueue.erase(it);
	Logger::debug("CGI tracker removed, marked inactive");
}

void CGIHandler::setClientToPollout(int fd, std::vector<struct pollfd>& poll_sets)
{
	for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end(); ++it)
	{
		if (it->fd == fd)
		{
			it->events = POLLOUT;
			break;
		}
	}
}

void CGIHandler::addPipeToPoll(int fd, short events, std::vector<struct pollfd>& poll_sets)
{
	// Fd Validation (safety check)
	if (fd <= 0)
	{
		Logger::debug("Skipping invalid FD " + Utils::toString(fd) + " for poll_sets");
		return;
	}

	// Check if this FD is already in the poll set
	for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end(); ++it)
	{
		if (it->fd == fd)
		{
			// Update events if needed instead of adding duplicate
			if (it->events != events)
			{
				Logger::debug("Updating events for FD " + Utils::toString(fd) + " in poll_sets");
				it->events = events;
			}
			return;
		}
	}

	// Add new FD to poll sets
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	poll_sets.push_back(pfd);
	Logger::debug("Added FD " + Utils::toString(fd) + " to poll_sets with events " + Utils::toString(events));
}

void CGIHandler::handleCGIWrite(int fd, std::vector<struct pollfd>& poll_sets)
{
	std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
	if (cgiIt == _cgiQueue.end())
	{
		Logger::warn("No CGI tracker found for FD " + Utils::toString(fd));
		return;
	}

	std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);
	if (pollIt == poll_sets.end())
		return;

	if (fd == cgiIt->cgi->getInPipeWriteFd())
	{
		// Write data to CGI input pipe
		cgiIt->cgi->writeInput();

		// If input is complete, remove pipe from pollset
		if (cgiIt->cgi->isInputDone())
		{
			cgiIt->cgi->closePipe(fd);
			poll_sets.erase(pollIt);
		}
	}
}

void CGIHandler::handleCGIHangup(int fd, std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients)
{
	std::vector<CGITracker>::iterator cgiIt = findCGI(fd);
	if (cgiIt == _cgiQueue.end())
	{
		Logger::info("No CGI tracker for FD " + Utils::toString(fd));
		return;
	}

	std::vector<struct pollfd>::iterator pollIt = findPollIterator(fd, poll_sets);
	if (pollIt == poll_sets.end()) return;

	int fdsToClose[3];
	initializeFDsToClose(cgiIt, fdsToClose);

	if (fd == cgiIt->cgi->getOutPipeReadFd())
	{
		Logger::info("CGI output complete on POLLHUP for FD " + Utils::toString(fd));
		std::string finalChunk = generateCGIResponse(cgiIt, "", true);
		setClientResponse(cgiIt, poll_sets, clients, finalChunk);
		removeCGIFDsFromPollSets(poll_sets, fdsToClose);
		closeCGIFDs(fdsToClose, cgiIt);
		cgiIt->isActive = false;
		removeCGI(cgiIt);
		Logger::debug("CGI cleanup completed for clientFd=" + Utils::toString(cgiIt->clientFd));
		Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
	}
	else
	{
		Logger::warn("Unexpected POLLHUP on FD " + Utils::toString(fd));
		handleCGIError(fd, poll_sets, clients);
	}
}

void CGIHandler::checkCGITimeout(std::vector<struct pollfd>& poll_sets, std::vector<ClientHandler>& clients, time_t currentTime)
{
	Logger::debug("Checking CGI timeouts, queue size: " + Utils::toString(_cgiQueue.size()));
	std::vector<CGITracker>::iterator cgiIt = _cgiQueue.begin();
	while (cgiIt != _cgiQueue.end())
	{
		// Skip invalid client FDs
		if (cgiIt->clientFd <= 0)
		{
			Logger::warn("Invalid client FD " + Utils::toString(cgiIt->clientFd) + " in CGI tracker for script " + cgiIt->cgi->getScriptPath() + ", marking inactive");
			cgiIt->isActive = false;
			++cgiIt;
			continue;
		}

		if (!cgiIt->isActive)
		{
			Logger::debug("Skipping inactive CGI for client FD " + Utils::toString(cgiIt->clientFd));
			++cgiIt;
			continue;
		}

		bool clientFound = false;
		std::vector<ClientHandler>::iterator clientIt = clients.begin();
		while (clientIt != clients.end())
		{
			if (clientIt->getFd() == cgiIt->clientFd)
			{
				clientFound = true;
				double cgiTimeout = cgiIt->serverConfig->getCGIProcessingTimeout();
				double elapsed = currentTime - clientIt->getTime();
				if (elapsed > cgiTimeout)
				{
					Logger::error("Client FD " + Utils::toString(cgiIt->clientFd) + " exceeded cgi_processing_timeout (" + Utils::toString(cgiTimeout) + "s) for script " + cgiIt->cgi->getScriptPath());

					// Terminate CGI process
					cgiIt->cgi->killIfTimedOut();
					cgiIt->cgi->setOutputDone();
					// Send final chunk if chunked
					if (cgiIt->isChunked)
					{
						std::string finalChunk = formatChunkedData("", true);
						ssize_t bytesWritten = write(cgiIt->clientFd, finalChunk.c_str(), finalChunk.length());
						if (bytesWritten != static_cast<ssize_t>(finalChunk.length()))
							Logger::error("Failed to send final chunk to FD " + Utils::toString(cgiIt->clientFd));
						else
							Logger::debug("Sent final chunk to FD " + Utils::toString(cgiIt->clientFd));
						// std::string finalChunk = "0\r\n\r\n";
						// ssize_t bytesWritten = write(cgiIt->clientFd, finalChunk.c_str(), finalChunk.length());
						// if (bytesWritten != static_cast<ssize_t>(finalChunk.length()))
						//     Logger::error("Failed to send final chunk to FD " + Utils::toString(cgiIt->clientFd));
						// else
						//     Logger::debug("Sent final chunk to FD " + Utils::toString(cgiIt->clientFd));
					}
					// Generate and send 503 response
					std::string response = generateServerTimeoutResponse(cgiIt);
					clientIt->setResponse(response);
					ssize_t bytesWritten = write(cgiIt->clientFd, response.c_str(), response.length());
					if (bytesWritten != static_cast<ssize_t>(response.length()))
						Logger::error("Failed to write 503 response to FD " + Utils::toString(cgiIt->clientFd));
					else
						Logger::debug("Wrote " + Utils::toString(bytesWritten) + " bytes of 503 response to FD " + Utils::toString(cgiIt->clientFd));


					// Clean up CGI resources
					int fdsToClose[3];
					initializeFDsToClose(cgiIt, fdsToClose);
					removeCGIFDsFromPollSets(poll_sets, fdsToClose);
					closeCGIFDs(fdsToClose, cgiIt);
					setClientToPollout(cgiIt->clientFd, poll_sets);
					cgiIt = _cgiQueue.erase(cgiIt);
					std::vector<CGITracker>::iterator toRemove = cgiIt;
					++cgiIt;
					removeCGI(toRemove);
					break;
				}
				else
					Logger::debug("FD: " + Utils::toString(cgiIt->clientFd) + " active CGI, elapsed: " + Utils::toString(elapsed) + ", timeout: " + Utils::toString(cgiTimeout));
				break;
			}
			++clientIt;
		}
		if (!clientFound)
		{
			Logger::warn("Client FD " + Utils::toString(cgiIt->clientFd) + " not found, cleaning up CGI for script " + cgiIt->cgi->getScriptPath());
			//bool clientFoundForResponse;
			//clientFoundForResponse = false;
			for (std::vector<ClientHandler>::iterator clIt = clients.begin(); clIt != clients.end(); ++clIt) {
				if (clIt->getFd() == cgiIt->clientFd)
				{
					HttpResponse response = createStandardResponse(504, "504 Gateway Timeout", "text/plain", true);
					std::string responseStr = response.composeResponse();
					clIt->setResponse(responseStr);
					ssize_t bytesWritten = write(cgiIt->clientFd, responseStr.c_str(), responseStr.length());
					if (bytesWritten != static_cast<ssize_t>(responseStr.length()))
						Logger::error("Failed to write 504 response to FD " + Utils::toString(cgiIt->clientFd));
					else
					{
						Logger::debug("Wrote " + Utils::toString(bytesWritten) + " bytes of 504 response to FD " +
									Utils::toString(cgiIt->clientFd));
					}
					//clientFoundForResponse = true;
					setClientToPollout(cgiIt->clientFd, poll_sets);
					break;
				}
			}
			int fdsToClose[3];
			initializeFDsToClose(cgiIt, fdsToClose);
			removeCGIFDsFromPollSets(poll_sets, fdsToClose);
			closeCGIFDs(fdsToClose, cgiIt);
			cgiIt->isActive = false;
			Logger::debug("CGI marked inactive for clientFd=" + Utils::toString(cgiIt->clientFd));
			++cgiIt;
		}
		else
			++cgiIt;
	}
}

void CGIHandler::killCGIForClient(int clientFd, std::vector<struct pollfd>& poll_sets)
{
	Logger::debug("Checking for CGI processes to kill for disconnected client FD " + Utils::toString(clientFd));

	for (std::vector<CGITracker>::iterator cgiIt = _cgiQueue.begin(); cgiIt != _cgiQueue.end(); )
	{
		if (cgiIt->clientFd == clientFd && cgiIt->isActive && cgiIt->cgi)
		{
			// Store FDs for cleanup
			int fdsToClose[3];
			initializeFDsToClose(cgiIt, fdsToClose);

			// Log the action (for loop.py)
			if (cgiIt->cgi->getScriptPath().find("loop.py") != std::string::npos)
			{
				Logger::info("╔══════════════════════════════════════════════════╗");
				Logger::info("║                                                  ║");
				Logger::info("║            💀 CLIENT DISCONNECT 💀               ║");
				Logger::info("║                                                  ║");
				Logger::info("║  ClientFD: " + Utils::toString(clientFd) +
				std::string(38 - Utils::toString(clientFd).length(), ' ') + "║");
				Logger::info("║  Script:   " + cgiIt->cgi->getScriptPath() +
				std::string(38 - cgiIt->cgi->getScriptPath().length(), ' ') + "║");
				Logger::info("║  Process:  " + Utils::toString(cgiIt->cgi->getProcessId()) +
				std::string(38 - Utils::toString(cgiIt->cgi->getProcessId()).length(), ' ') + "║");
				Logger::info("║                                                  ║");
				Logger::info("║           CONNECTION TERMINATED                  ║");
				Logger::info("║                                                  ║");
				Logger::info("╚══════════════════════════════════════════════════╝");
			}
			else
			{
				Logger::info("Client FD " + Utils::toString(clientFd) +
							" disconnected, terminating CGI process " +
							Utils::toString(cgiIt->cgi->getProcessId()) + " for script " +
							cgiIt->cgi->getScriptPath());
			}

			// Kill the CGI process immediately with SIGKILL to ensure it stops
			if (cgiIt->cgi->getProcessId() > 0)
			{
				kill(cgiIt->cgi->getProcessId(), SIGKILL);
				Logger::info("Terminated CGI process " + Utils::toString(cgiIt->cgi->getProcessId()) +
							" for client FD " + Utils::toString(clientFd));
			}

			// Clean up all file descriptors
			removeCGIFDsFromPollSets(poll_sets, fdsToClose);
			closeCGIFDs(fdsToClose, cgiIt);

			Logger::debug("Deleting CGI object for FD " + Utils::toString(clientFd));
			//delete cgiIt->cgi;
			//cgiIt->cgi = NULL;
			cgiIt->isActive = false;
			// Remove CGI tracker completely
			cgiIt = _cgiQueue.erase(cgiIt);
			Logger::info("CGI tracker removed for disconnected client FD " + Utils::toString(clientFd));
		}
		else
			++cgiIt;
	}
	Logger::debug("Finished killing CGI processes, _cgiQueue size: " + Utils::toString(_cgiQueue.size()));
}

HttpResponse CGIHandler::createStandardResponse(int statusCode, const std::string& body,
	const std::string& contentType, bool closeConnection)
{
	HttpResponse response(statusCode, body);
	response.setHeader("Content-Type", contentType);
	response.setHeader("Content-Length", Utils::toString(body.length()));
	response.setHeader("Cache-Control", "no-cache");

	if (closeConnection)
		response.setHeader("Connection", "close");
	else
		response.setHeader("Connection", "keep-alive");

	response.setHeader("Date", response.findTimeStamp());
	return response;
}

// Little helpers
bool CGIHandler::isUploadScript(const std::string& path) const
{
	return path.find("upload.py") != std::string::npos;
}

bool CGIHandler::isLoopScript(const std::string& path) const
{
	return path.find("loop.py") != std::string::npos;
}

bool CGIHandler::isFortuneScript(const std::string& path) const
{
	return path.find("fortune.php") != std::string::npos;
}

std::string CGIHandler::formatChunkSize(size_t size) const
{
	std::ostringstream hex;
	hex << std::hex << size;
	return hex.str();
}

std::string CGIHandler::formatChunkedData(const std::string& content, bool isFinal) const
{
	std::ostringstream chunk;
	// Format content chunk if there is content
	if (!content.empty())
	{
		std::string hexSize = formatChunkSize(content.length());
		chunk << hexSize << "\r\n" << content << "\r\n";
	}
	// Add final chunk marker if this is the final chunk
	if (isFinal) { chunk << "0\r\n\r\n"; }
	return chunk.str();
}

// void CGIHandler::cleanupInactiveCGIFDs(std::vector<struct pollfd>& poll_sets)
// {
//     Logger::debug("Starting cleanup of inactive CGI FDs");

//     // Collect all valid CGI FDs
//     std::vector<int> activeFDs;

//     // First, collect active CGI FDs
//     for (std::vector<CGITracker>::iterator cgiIt = _cgiQueue.begin();
//          cgiIt != _cgiQueue.end(); ++cgiIt)
//     {
//         if (cgiIt->isActive && cgiIt->cgi)
//         {
//             int fds[3] = {
//                 cgiIt->cgi->getInPipeWriteFd(),
//                 cgiIt->cgi->getOutPipeReadFd(),
//                 cgiIt->cgi->getErrPipeReadFd()
//             };
//             // Check if the CGI FDs are valid and add them to the activeFDs list
//             for (int i = 0; i < 3; ++i) { if (fds[i] > 0) { activeFDs.push_back(fds[i]); } }
//         }
//     }

//     // Remove any FDs that aren't in the active list but are in poll_sets
//     for (std::vector<struct pollfd>::iterator it = poll_sets.begin(); it != poll_sets.end();)
//     {
//         bool isCGIFd = false;
//         bool isActive = false;

//         // Check if this is a CGI FD by looking at the stored FDs
//         for (size_t i = 0; i < activeFDs.size(); ++i)
//         {
//             if (it->fd == activeFDs[i])
//             {
//                 isCGIFd = true;
//                 isActive = true;
//                 break;
//             }
//         }

//         // If not found in active list, check if it's a CGI FD that's inactive
//         if (!isCGIFd)
//         {
//             // This heuristic checks if it's likely a CGI FD (not a client or server socket)
//             if (it->fd > 2 && it->fd < 1000)  // Typical range for pipe FDs
//             {
//                 struct stat statbuf;
//                 if (fstat(it->fd, &statbuf) == 0 && S_ISFIFO(statbuf.st_mode))
//                 {
//                     isCGIFd = true;
//                 }
//             }
//         }

//         // If it's an inactive CGI FD, remove it from the poll set
//         if (isCGIFd && !isActive)
//         {
//             Logger::debug("Removing stale CGI FD " + Utils::toString(it->fd) + " from poll_sets");
//             close(it->fd);  // Close the FD
//             it = poll_sets.erase(it);  // Remove from poll_sets
//         }
//         else
//             ++it;
//     }
//     Logger::debug("Finished cleaning up inactive CGI FDs");
// }

std::string CGIHandler::generateServerErrorResponse(std::vector<CGITracker>::iterator cgiIt, int statusCode, const std::string& statusText)
{
	std::string scriptPath = cgiIt->cgi->getScriptPath();
	bool isUploadPy = isUploadScript(scriptPath);
	bool isLoopPy = isLoopScript(scriptPath);
	std::string errorBody;
	std::string contentType;

	// Determine response format based on script type
	if (isUploadPy)
	{
		std::ostringstream json;
		json << "{\"error\": \"" << statusText << "\", \"code\": " << statusCode << "}";
		errorBody = json.str();
		contentType = "application/json";
	} else if (isLoopPy) {
		std::ostringstream text;
		text << statusCode << " " << statusText;
		errorBody = text.str();
		contentType = "text/plain";
	} else {
		errorBody = HttpException(statusCode, statusText).getBody();
		contentType = "text/html";
	}

	// Create response
	HttpResponse response = createStandardResponse(statusCode, errorBody, contentType, true);
	// for (std::map<std::string, std::string>::iterator it = cgiIt->cgi_headers.begin();
	//      it != cgiIt->cgi_headers.end(); ++it) {
	//     if (it->first != "Transfer-Encoding") {
	//         response.setHeader(it->first, it->second);
	//     }
	// }
	// Explicitly clear any conflicting headers
	cgiIt->cgi_headers.clear();
	response.setHeader("Connection", "close"); // Ensure connection closure
	response.setHeader("Content-Type", contentType);
	response.setHeader("Content-Length", Utils::toString(errorBody.length()));
	response.setHeader("Cache-Control", "no-cache");
	response.setHeader("Date", response.findTimeStamp());

	std::string fullResponse = response.composeResponse();
	Logger::error(Utils::toString(statusCode) + " " + statusText + " sent for script " + scriptPath);
	cgiIt->firstResponseSent = true;
	cgiIt->bufferedOutput.clear();
	return fullResponse;
}
