#ifndef CGI_HPP
#define CGI_HPP

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
#include <sys/poll.h>

class ClientRequest;
class Server;

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
        char** _av;
        char** _env;
        std::string _output;
        int _in_pipe[2];
        int _out_pipe[2];
        pid_t _pid;
        size_t _bytes_written;
        bool _input_done;
        bool _output_done;
        time_t _start_time;
        //removed Poll_Fds (we use poll_sets in SocketServer directly)

        static const std::string SCRIPT_BASE_DIR;
        static const int TIMEOUT_SECONDS = 5;

        std::string getScriptFileName(const ClientRequest& request, const Server& server) const;
        void createAvAndEnv(const ClientRequest& request, const Server& server);
        bool unchunkRequest(const std::string& chunked, std::string& unchunked);
        void processRequestBody(const ClientRequest& request);
        void runScript();
        void populateEnvVariables(const ClientRequest& request, const Server& server);
        void cleanup();


    public:
        
        CGI(const ClientRequest& request, const std::string& upload_to, const Server& server);
        ~CGI();

        //Getters
        std::string getOutput() const { return _output; }
        int getInPipeWriteFd() const { return _in_pipe[1]; }
        int getOutPipeReadFd() const { return _out_pipe[0]; }
        pid_t getPid() const { return _pid; }
        const std::string& getProcessedBody() const { return _processed_body; }
        // Removed getPollFds() // we dont have _poll_fds anymore

        //Helpers
        void appendOutput(const char* data, size_t len);
        void updateBytesWritten(size_t bytes);
        bool isDone() const;
        bool isTimedOut() const;
        void closePipes();
        // Removed init_PollFds() / ServerSocket handles it directly
};

#endif