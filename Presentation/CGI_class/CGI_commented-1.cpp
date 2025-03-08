#include "CGI.hpp"
#include "Logger.hpp"

// Static constants for the CGI class
const std::string CGI::SCRIPT_BASE_DIR = "./cgi_bin/"; // Base directory where CGI scripts (e.g., Python files) live
const int CGI::TIMEOUT_SECONDS = 20;                   // Max time (in seconds) a CGI script can run before being killed

// Constructor: Initializes the CGI object with request data and starts execution
CGI::CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& serverInfo, const std::string& raw_body)
    : _request_method(request.getRequestLine().find("Method")->second), // Extract HTTP method (e.g., GET, POST)
      _upload_to(upload_to),                                    // Directory where uploads will go
      _processed_body(raw_body),                                // Raw request body to pass to the script
      _output(""),                                              // Will store script output
      _av(NULL),                                                // Argument vector for execve (initially null)
      _env(NULL),                                               // Environment variables for execve (initially null)
      _in_pipe(),                                               // Pipe for sending input to script
      _out_pipe(),                                              // Pipe for receiving output from script
      _pid(-1),                                                 // Process ID of the forked script (-1 = not started)
      _stderr_fd(-1),                                           // File descriptor for script's stderr
      _bytes_written(0),                                        // Tracks how much of the body we've written
      _input_done(false),                                       // Flag: Have we finished sending input?
      _output_done(false),                                      // Flag: Have we finished reading output?
      _start_time(time(NULL)),                                  // Record when we start for timeout checking
      _serverInfo(serverInfo)                                   // Server config details
{
    // Initialize pipe file descriptors to -1 (not open yet)
    _in_pipe[0] = -1; _in_pipe[1] = -1;  // Input pipe: [0] read end, [1] write end
    _out_pipe[0] = -1; _out_pipe[1] = -1; // Output pipe: [0] read end, [1] write end

    try 
    {
        // Step 1: Set up environment variables based on the request
        populateEnvVariables(request);
        // Step 2: Figure out which script to run (e.g., ./cgi_bin/upload.py)
        _cgi_path = getScriptFileName(request);
        // Step 3: Prepare arguments and environment for execution
        createAvAndEnv(request);
        // Log the size of the request body for debugging
        std::ostringstream oss;
        oss << _processed_body.size();
        Logger::debug("Raw body size: " + oss.str());
        // Step 4: Start running the script in a child process
        startExecution();
    } 
    catch (const std::exception& e) 
    {
        // If anything fails, clean up resources and rethrow the exception
        cleanup();
        throw;
    }
}

// Destructor: Ensures all resources are cleaned up when the object is destroyed
CGI::~CGI() 
{
    cleanup(); // Call cleanup to free memory, close pipes, and kill the process if running
}

// Cleanup function: Frees memory and closes resources
void CGI::cleanup() 
{
    // Free the argument vector if it exists
    if (_av) 
    {
        for (size_t i = 0; _av[i]; ++i) free(_av[i]); // Free each string
        delete[] _av;                                 // Free the array itself
        _av = NULL;                                   // Reset to null
    }
    // Free the environment vector if it exists
    if (_env) 
    {
        for (size_t i = 0; _env[i]; ++i) free(_env[i]); // Free each string
        delete[] _env;                                  // Free the array itself
        _env = NULL;                                    // Reset to null
    }
    closePipes();           // Close all open pipes
    closePipe(_stderr_fd);  // Close stderr file descriptor if open
    // If a child process is running, kill it and wait for it to finish
    if (_pid > 0) 
    {
        kill(_pid, SIGKILL); // Send kill signal to stop the process
        waitpid(_pid, NULL, 0); // Wait for it to terminate
        _pid = -1;           // Reset PID
    }
}

// Determines the script file to run based on the request URI
std::string CGI::getScriptFileName(const ClientRequest& request) const 
{
    std::string uri = request.getRequestLine().find("Request-URI")->second; // Get the URI (e.g., "/upload" or "/script.py")
    std::string cgi_root = SCRIPT_BASE_DIR; // Base dir for scripts (./cgi_bin/)
    // Special case: if URI is "/upload", use upload.py
    if (uri == "/upload") 
    {
        std::string script_path = cgi_root + "upload.py";
        if (access(script_path.c_str(), F_OK) != 0) // Check if file exists
            throw CGIException("Script file does not exist: " + script_path);
        return script_path;
    }
    // General case: extract script name from URI
    size_t pos = uri.find_last_of('/'); // Find last slash
    std::string script_name = (pos != std::string::npos) ? uri.substr(pos + 1) : uri; // Get part after last slash
    size_t qpos = script_name.find('?'); // Remove query string if present
    if (qpos != std::string::npos) script_name = script_name.substr(0, qpos);
    // Validate the script name
    if (script_name.empty() || script_name.find('.') == std::string::npos || script_name.substr(script_name.find_last_of('.')) != ".py")
        throw CGIException("Invalid script extension. Only .py scripts are supported.");
    std::string full_path = cgi_root + script_name; // Build full path
    if (access(full_path.c_str(), F_OK) != 0)       // Check if file exists
        throw CGIException("Script file does not exist: " + full_path);
    return full_path;
}

// Sets up the argument vector (av) for execve
void CGI::createAv(const std::string& cgi_path, const std::string& upload_to) 
{
    // Handle empty upload directory by defaulting to ./uploads
    if (upload_to.empty())
    {
        Logger::warn("Upload directory is empty, defaulting to './uploads'");
        _upload_to = "./uploads"; // Set default
    }
    // Create upload directory if it doesn’t exist
    if (!opendir(_upload_to.c_str()))
    {
        Logger::debug("Creating upload directory: " + _upload_to);
        mkdir(_upload_to.c_str(), 0755); // Permissions: rwxr-xr-x
    }
    // Allocate and populate the argument vector
    _av = new char*[4];                     // 4 elements: interpreter, script, upload dir, null terminator
    _av[0] = strdup("/usr/bin/python3");    // Python interpreter
    _av[1] = strdup(cgi_path.c_str());      // Path to the script
    _av[2] = strdup(upload_to.c_str());     // Upload directory
    _av[3] = NULL;                          // Null terminator for execve
    Logger::debug("CGI arguments: [" + std::string(_av[0]) + ", " + _av[1] + ", " + _av[2] + "]");
}

// Sets up the environment variables for execve
void CGI::createEnv(const ClientRequest& request) 
{
    std::map<std::string, std::string> headers = request.getHeaders();     // HTTP headers
    std::map<std::string, std::string> queryMap = request.getUriQueryMap(); // Query parameters

    // Calculate size needed for environment array
    size_t env_size = _env_map.size() + headers.size() + queryMap.size();
    _env = new char*[env_size + 1]; // +1 for null terminator
    size_t i = 0;
    // Add base environment variables from _env_map
    for (std::map<std::string, std::string>::const_iterator it = _env_map.begin(); it != _env_map.end(); ++it) 
    {
        std::string env_entry = it->first + "=" + it->second; // e.g., "REQUEST_METHOD=POST"
        _env[i++] = strdup(env_entry.c_str());
    }
    // Add HTTP headers with "HTTP_" prefix
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) 
    {
        std::string env_entry = "HTTP_" + it->first + "=" + it->second; // e.g., "HTTP_Content-Type=application/json"
        _env[i++] = strdup(env_entry.c_str());
    }
    // Add query parameters with "QUERY_" prefix
    for (std::map<std::string, std::string>::const_iterator it = queryMap.begin(); it != queryMap.end(); ++it) 
    {
        std::string env_entry = "QUERY_" + it->first + "=" + it->second; // e.g., "QUERY_id=123"
        _env[i++] = strdup(env_entry.c_str());
    }
    _env[i] = NULL; // Null terminator for execve
}

// Combines argument and environment setup, with a safety check
void CGI::createAvAndEnv(const ClientRequest& request) 
{
    if (access(_cgi_path.c_str(), F_OK) != 0) // Double-check script exists
        throw CGIException("Script file does not exist: " + _cgi_path);
    createAv(_cgi_path, _upload_to); // Set up arguments
    createEnv(request);              // Set up environment
}

// Handles chunked encoding in POST requests, converting to plain data
bool CGI::unchunkRequest(const std::string& chunked, std::string& unchunked) 
{
    std::istringstream iss(chunked); // Stream to parse chunked data
    std::string line;
    long chunk_size;
    while (std::getline(iss, line)) 
    {
        if (line.empty()) continue; // Skip empty lines
        char* end_ptr;
        chunk_size = strtol(line.c_str(), &end_ptr, 16); // Parse chunk size in hex
        if (*end_ptr != '\0' || chunk_size < 0) throw CGIException("Invalid chunk size");
        if (chunk_size == 0) break; // End of chunks
        std::vector<char> buffer(chunk_size); // Buffer for chunk data
        iss.read(&buffer[0], chunk_size);     // Read chunk
        if (iss.gcount() != static_cast<std::streamsize>(chunk_size))
            throw CGIException("Failed to read chunk data");
        unchunked.append(&buffer[0], chunk_size); // Append to output
    }
    return !unchunked.empty(); // True if we got any data
}

// Populates the environment map with request details
void CGI::populateEnvVariables(const ClientRequest& request) 
{
    std::map<std::string, std::string> requestLine = request.getRequestLine();
    std::string uri = requestLine.find("Request-URI")->second; // e.g., "/script.py?id=123"
    size_t qpos = uri.find('?'); // Split URI into path and query
    if (qpos != std::string::npos) 
    {
        _env_map["QUERY_STRING"] = uri.substr(qpos + 1); // e.g., "id=123"
        _env_map["PATH_INFO"] = uri.substr(0, qpos);     // e.g., "/script.py"
    } 
    else 
    {
        _env_map["QUERY_STRING"] = ""; // No query
        _env_map["PATH_INFO"] = uri;   // Full URI is the path
    }
    _env_map["REQUEST_METHOD"] = _request_method;         // e.g., "POST"
    _env_map["REQUEST_URI"] = uri;                        // Full URI
    _env_map["SERVER_PROTOCOL"] = requestLine.find("Protocol")->second; // e.g., "HTTP/1.1"
    _env_map["SERVER_NAME"] = "127.0.0.1";                // Localhost
    std::vector<std::string> ports = _serverInfo.getArrayPorts();
    _env_map["SERVER_PORT"] = ports.empty() ? "8080" : ports[0]; // Default port or first available
    _env_map["GATEWAY_INTERFACE"] = "CGI/1.1";            // CGI version
    // Handle POST-specific environment variables
    if (_request_method == "POST") 
    {
        std::ostringstream oss;
        oss << _processed_body.size();
        _env_map["CONTENT_LENGTH"] = oss.str(); // Size of body
        std::map<std::string, std::string> headers = request.getHeaders();
        std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
        if (it != headers.end()) 
        {
            _env_map["CONTENT_TYPE"] = it->second; // e.g., "application/json"
            Logger::debug("CGI CONTENT_TYPE set to: " + it->second);
        }
        it = headers.find("Transfer-Encoding");
        if (it != headers.end() && it->second == "chunked") // Handle chunked POST data
        {
            std::string unchunked;
            if (unchunkRequest(_processed_body, unchunked)) 
            {
                _processed_body = unchunked; // Replace with unchunked data
                oss.str(""); oss << _processed_body.size();
                _env_map["CONTENT_LENGTH"] = oss.str(); // Update length
            }
        }
    } 
    else
        _env_map["CONTENT_LENGTH"] = "0"; // No body for non-POST
}

// Starts the CGI script execution in a child process
void CGI::startExecution()
{
    int stderr_pipe[2]; // Pipe for capturing stderr
    Logger::debug("Creating pipes for CGI");
    // Create pipes for input, output, and stderr
    if (pipe(_in_pipe) == -1 || pipe(_out_pipe) == -1 || pipe(stderr_pipe) == -1)
    {
        closePipes(); close(stderr_pipe[0]); close(stderr_pipe[1]);
        throw CGIException("Failed to create pipes");
    }
    Logger::debug("Forking CGI process");
    _pid = fork(); // Split into parent and child
    if (_pid == -1) // Fork failed
    {
        closePipes(); close(stderr_pipe[0]); close(stderr_pipe[1]);
        throw CGIException("Failed to fork");
    }
    if (_pid == 0) // Child process
    {
        close(_in_pipe[1]); close(_out_pipe[0]); close(stderr_pipe[0]); // Close unused ends
        // Redirect standard I/O to pipes
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 ||
            dup2(_out_pipe[1], STDOUT_FILENO) == -1 ||
            dup2(stderr_pipe[1], STDERR_FILENO) == -1)
            exit(1); // Exit if redirection fails
        close(_in_pipe[0]); close(_out_pipe[1]); close(stderr_pipe[1]); // Close original FDs
        execve(_av[0], _av, _env); // Run the script (e.g., python3 script.py)
        exit(1); // Exit if execve fails
    }
    else // Parent process
    {
        Logger::debug("In parent, closing unused pipe ends");
        close(_in_pipe[0]); close(_out_pipe[1]); close(stderr_pipe[1]); // Close unused ends
        _out_pipe[1] = -1; // Mark write end as closed
        // If no body to send, close input pipe immediately
        if (_processed_body.empty())
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("No body to write, input done");
        }
        _stderr_fd = stderr_pipe[0]; // Save stderr read end for polling
        std::ostringstream oss;
        oss << "CGI stderr FD stored for polling: " << _stderr_fd;
        Logger::debug(oss.str());
    }
}

// Appends data to the output string (called when reading script output)
void CGI::appendOutput(const char* data, size_t len) 
{
    _output.append(data, len); // Add script output to _output
}

// Writes the request body to the script’s stdin
void CGI::writeInput()
{
    Logger::debug("Entering writeInput");
    if (_input_done || _in_pipe[1] == -1) // Skip if done or pipe is closed
    {
        Logger::debug("CGI writeInput skipped: already done or pipe closed");
        return;
    }
    size_t remaining = _processed_body.size() - _bytes_written; // Bytes left to write
    if (remaining <= 0) // Nothing left to write
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        Logger::debug("CGI input completed (no remaining bytes)");
        return;
    }
    const size_t CHUNK_SIZE = 1024; // Write in 1KB chunks
    size_t to_write = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
    std::ostringstream oss;
    oss << "Writing " << to_write << " bytes to CGI";
    Logger::debug(oss.str());
    ssize_t written = write(_in_pipe[1], _processed_body.c_str() + _bytes_written, to_write);
    if (written > 0) // Successfully wrote some bytes
    {
        _bytes_written += written;
        oss.str(""); oss << "Wrote " << written << " of " << remaining << " bytes to CGI";
        Logger::debug(oss.str());
        if (_bytes_written >= _processed_body.size()) // All bytes written
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("CGI input fully written and pipe closed");
        }
    }
    else // Write failed or pipe closed
    {
        oss.str(""); oss << "CGI write failed or pipe closed, written: " << (written == 0 ? "0" : "-1");
        Logger::debug(oss.str());
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
    }
}

// Closes a single pipe file descriptor
void CGI::closePipe(int& pipe_fd) 
{
    if (pipe_fd != -1) // Only close if it’s open
    {
        close(pipe_fd);
        pipe_fd = -1; // Mark as closed
    }
}

// Closes all pipe file descriptors
void CGI::closePipes() 
{
    closePipe(_in_pipe[0]); closePipe(_in_pipe[1]);
    closePipe(_out_pipe[0]); closePipe(_out_pipe[1]);
}

// Checks if the script has timed out
bool CGI::isTimedOut() const 
{
    return time(NULL) - _start_time >= TIMEOUT_SECONDS; // True if past timeout
}

// Kills the script if it’s timed out
void CGI::killIfTimedOut() 
{
    if (isTimedOut() && _pid > 0) // Only kill if running and timed out
    {
        kill(_pid, SIGKILL); // Send kill signal
        waitpid(_pid, NULL, 0); // Wait for termination
        _pid = -1;           // Reset PID
        _input_done = true;  // Mark input/output as done
        _output_done = true;
        closePipes();        // Close all pipes
        Logger::warn("CGI process killed due to timeout");
    }
}