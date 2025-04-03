#ifndef CGI_HPP
#define CGI_HPP

#include "HttpRequest.hpp"
#include "Config.hpp"
#include "Logger.hpp"
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
#include <dirent.h>
#include <sys/stat.h>

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
        int _err_pipe[2];
        pid_t _pid;
        size_t _bytes_written;
        bool _input_done;
        bool _output_done;
        time_t _start_time;
        std::map<std::string, std::string> _env_map;
        const InfoServer& _serverInfo;

        std::string getScriptFileName(const HttpRequest& request) const;
        void createAv(const std::string& cgi_path, const std::string& upload_to);
        void createEnv(const HttpRequest& request);
        void createAvAndEnv(const HttpRequest& request);
        bool unchunkRequest(const std::string& chunked, std::string& unchunked);
        void populateEnvVariables(const HttpRequest& request);
        void startExecution();
        void closePipes();

    public:
        static const std::string SCRIPT_BASE_DIR;
        static const int TIMEOUT_SECONDS;

        CGI(const HttpRequest& request, const std::string& upload_to, const InfoServer& info, const std::string& raw_body);
        ~CGI();

        int getInPipeWriteFd() const { return _in_pipe[1]; }
        int getOutPipeReadFd() const { return _out_pipe[0]; }
        int getErrPipeReadFd() const { return _err_pipe[0]; }
        pid_t getPid() const { return _pid; }
        std::string getOutput() const { return _output; }
        const std::string& getProcessedBody() const { return _processed_body; }
        void setOutputDone(); // new

        bool isInputDone() const { return _input_done; }
        bool isOutputDone() const { return _output_done; }
        bool isTimedOut() const;

        void appendOutput(const char* data, size_t len);
        void writeInput();
        void closePipe(int& pipe_fd);
        void killIfTimedOut();
};

#endif // CGI_HPP