#ifndef CGI_HPP
#define CGI_HPP

#include "ClientRequest.hpp"
#include "InfoServer.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <map>
#include <sstream>
#include <dirent.h>    // Added for opendir
#include <sys/stat.h>  // Added for mkdir

class ClientRequest;

class CGIException : public std::runtime_error {
public:
    explicit CGIException(const std::string& message) : std::runtime_error(message) {}
};

class CGI {
private:
    std::string _request_method;
    std::string _upload_to;
    std::string _cgi_path;
    std::string _processed_body; // Raw body (binary-safe for images)
    std::string _output;        // CGI script output
    char** _av;
    char** _env;
    int _in_pipe[2];  // Write to CGI stdin
    int _out_pipe[2]; // Read from CGI stdout
    pid_t _pid;
    int _stderr_fd;   // Added for stderr polling
    size_t _bytes_written; // Track internally
    bool _input_done;
    bool _output_done;
    time_t _start_time;
    std::map<std::string, std::string> _env_map;
    const InfoServer& _serverInfo;

    // Helpers
    std::string getScriptFileName(const ClientRequest& request) const;
    void createAv(const std::string& cgi_path, const std::string& upload_to);
    void createEnv(const ClientRequest& request);
    void createAvAndEnv(const ClientRequest& request);
    bool unchunkRequest(const std::string& chunked, std::string& unchunked);
    void populateEnvVariables(const ClientRequest& request);
    void startExecution();
    void closePipes();
    void cleanup();

public:
    static const std::string SCRIPT_BASE_DIR;
    static const int TIMEOUT_SECONDS;

    CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& info, const std::string& raw_body);
    ~CGI();

    // Getters
    int getInPipeWriteFd() const { return _in_pipe[1]; }
    int getOutPipeReadFd() const { return _out_pipe[0]; }
    int getStderrFd() const { return _stderr_fd; }
    pid_t getPid() const { return _pid; }
    std::string getOutput() const { return _output; }
    const std::string& getProcessedBody() const { return _processed_body; }
    bool isInputDone() const { return _input_done; }
    bool isOutputDone() const { return _output_done; }
    bool isTimedOut() const;

    // Helpers
    void appendOutput(const char* data, size_t len);
    void writeInput();
    void closePipe(int& pipe_fd);
    void killIfTimedOut();
};

#endif // CGI_HPP