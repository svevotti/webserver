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
#include <cerrno>
#include <climits>
#include <sstream>


class ClientRequest;
class InfoServer;


class CGIException : public std::runtime_error {

    public:

        explicit CGIException(const std::string& message) : std::runtime_error(message) {}

};


class CGI {

    private:

        std::string _request_method;
        std::string _upload_to;
        std::string _cgi_path;
        std::string _processed_body;
        std::string _output;
        char** _av;
        char** _env;
        int _in_pipe[2];
        int _out_pipe[2];
        pid_t _pid;
        size_t _bytes_written;
        bool _input_done;
        bool _output_done;
        time_t _start_time;
        std::map<std::string, std::string> _env_map;

        //Helpers
        std::string getScriptFileName(const ClientRequest& request, const InfoServer& info) const;
        void createAv(const std::string& cgi_path, const std::string& upload_to);
        void createEnv(const ClientRequest& request, const InfoServer& info);
        void createAvAndEnv(const ClientRequest& request, const InfoServer& info);
        bool unchunkRequest(const std::string& chunked, std::string& unchunked);
        void processRequestBody(const ClientRequest& request);
        void populateEnvVariables(const ClientRequest& request, const InfoServer& info);
        void startExecution();
    
        void closePipes();
        void cleanup();


    public:

        // Constants
        static const std::string SCRIPT_BASE_DIR;
        static const int TIMEOUT_SECONDS;

        // Constructor and Destructor
        CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& info);
        ~CGI();

        // Getters
        int getInPipeWriteFd() const { return _in_pipe[1]; }
        int getOutPipeReadFd() const { return _out_pipe[0]; }
        pid_t getPid() const { return _pid; }
        std::string getOutput() const { return _output; }
        const std::string& getProcessedBody() const { return _processed_body; }
        bool isInputDone() const { return _input_done; }
        bool isOutputDone() const { return _output_done; }
        bool isDone() const;
        bool isTimedOut() const;

        // Helpers
        void appendOutput(const char* data, size_t len);
        void updateBytesWritten(size_t bytes);
        void writeInput(const std::string& data, int fd); 
        //void closePipe(int& pipe_fd);
        void closePipe(int pipe_fd); // or use the former and manage it internally

        // Setter
        void setOutputDone(bool done) { _output_done = done; }  // Added for Implementing writeInput in CGI for a cleaner separation of concerns.
   
};

#endif