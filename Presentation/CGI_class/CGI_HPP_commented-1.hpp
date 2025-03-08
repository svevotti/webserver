#ifndef CGI_HPP // Include guard to prevent multiple inclusions
#define CGI_HPP

#include "ClientRequest.hpp" // For handling HTTP client requests
#include "InfoServer.hpp"    // For server configuration details
#include <iostream>          // For std::cerr (debugging output)
#include <vector>            // For std::vector (used in unchunking)
#include <string>            // For std::string (core data type)
#include <stdexcept>         // For std::runtime_error (base for exceptions)
#include <unistd.h>          // For POSIX functions (e.g., pipe, fork)
#include <cstdlib>           // For standard C functions (e.g., free)
#include <cstring>           // For C-string operations (e.g., strdup)
#include <sys/types.h>       // For pid_t and other system types
#include <sys/wait.h>        // For waitpid (child process management)
#include <map>               // For std::map (environment variables)
#include <sstream>           // For std::ostringstream (logging)
#include <dirent.h>          // For opendir (directory operations)
#include <sys/stat.h>        // For mkdir (directory creation)

// Forward declaration to avoid circular dependency
class ClientRequest;

// Custom exception class for CGI-specific errors
class CGIException : public std::runtime_error {
public:
    // Constructor: Takes a message and passes it to std::runtime_error
    explicit CGIException(const std::string& message) : std::runtime_error(message) {}
};

// CGI class: Manages execution of CGI scripts (e.g., Python scripts)
class CGI {
private:
    // Member variables
    std::string _request_method;           // HTTP method (e.g., GET, POST)
    std::string _upload_to;                // Directory for uploaded files
    std::string _cgi_path;                 // Full path to the CGI script
    std::string _processed_body;           // Raw request body (binary-safe for images, etc.)
    std::string _output;                   // Output from the CGI script
    char** _av;                            // Argument vector for execve (e.g., {"python3", "script.py", ...})
    char** _env;                           // Environment variables for execve (e.g., {"REQUEST_METHOD=POST", ...})
    int _in_pipe[2];                       // Pipe for sending input to CGI stdin ([0] read, [1] write)
    int _out_pipe[2];                      // Pipe for reading CGI stdout ([0] read, [1] write)
    pid_t _pid;                            // Process ID of the forked CGI process
    int _stderr_fd;                        // File descriptor for CGI stderr (for error logging)
    size_t _bytes_written;                 // Tracks bytes written to CGI stdin
    bool _input_done;                      // Flag: Has all input been sent to CGI?
    bool _output_done;                     // Flag: Has all output been read from CGI?
    time_t _start_time;                    // Time when CGI execution started (for timeout)
    std::map<std::string, std::string> _env_map; // Map of environment variables before conversion to char**
    const InfoServer& _serverInfo;         // Reference to server configuration

    // Private helper methods
    std::string getScriptFileName(const ClientRequest& request) const; // Determines script path from request URI
    void createAv(const std::string& cgi_path, const std::string& upload_to); // Sets up argument vector
    void createEnv(const ClientRequest& request);                     // Sets up environment variables
    void createAvAndEnv(const ClientRequest& request);                // Combines av and env setup
    bool unchunkRequest(const std::string& chunked, std::string& unchunked); // Handles chunked POST data
    void populateEnvVariables(const ClientRequest& request);          // Fills _env_map with request data
    void startExecution();                                            // Forks and executes the CGI script
    void closePipes();                                                // Closes all pipe FDs
    void cleanup();                                                   // Frees resources (called by destructor or on error)

public:
    // Static constants
    static const std::string SCRIPT_BASE_DIR; // Base directory for CGI scripts (e.g., "./cgi_bin/")
    static const int TIMEOUT_SECONDS;         // Timeout for CGI execution (e.g., 20 seconds)

    // Constructor: Initializes CGI object and starts execution
    CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& info, const std::string& raw_body);
    
    // Destructor: Ensures cleanup of resources
    ~CGI();

    // Public getters (const-safe access to private members)
    int getInPipeWriteFd() const { return _in_pipe[1]; }   // Returns FD for writing to CGI stdin
    int getOutPipeReadFd() const { return _out_pipe[0]; }  // Returns FD for reading CGI stdout
    int getStderrFd() const { return _stderr_fd; }         // Returns FD for reading CGI stderr
    pid_t getPid() const { return _pid; }                  // Returns PID of CGI process
    std::string getOutput() const { return _output; }      // Returns CGI script output
    const std::string& getProcessedBody() const { return _processed_body; } // Returns raw request body
    bool isInputDone() const { return _input_done; }       // Checks if input is fully sent
    bool isOutputDone() const { return _output_done; }     // Checks if output is fully read
    bool isTimedOut() const;                               // Checks if execution has timed out

    // Public helper methods
    void appendOutput(const char* data, size_t len);       // Appends data to _output (called when reading stdout)
    void writeInput();                                     // Writes _processed_body to CGI stdin
    void closePipe(int& pipe_fd);                          // Closes a single pipe FD
    void killIfTimedOut();                                 // Kills CGI process if it exceeds timeout
};

#endif // CGI_HPP