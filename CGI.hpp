#ifndef CGI_HPP
#define CGI_HPP

#include "HttpRequest.hpp"
#include "ClientHandler.hpp"
#include "HttpException.hpp"
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
#include <fstream>

class CGIException : public std::runtime_error {

    public:
        explicit CGIException(const std::string& message) : std::runtime_error(message) {}

};

class CGI {

	private:
		std::string _request_method;
		std::string	_upload_dir;
		std::string _cgi_path;
		std::string _processed_body;
		std::string _output;
		char** _av;
		char** _env;
		int _fd;
		pid_t _pid;
		std::string	_uri;
		const InfoServer& _serverInfo;
		std::map<std::string, std::string> _env_map;
		// size_t _bytes_written;
		// bool _input_done;
		// bool _output_done;
		// time_t _start_time;
		// double _timeout;
		// std::string _error_output; // Store STDERR output
		// bool _timed_out; // Track timeout state
		// std::string _timeout_response; // Store timeout response

		void createAv();
		void createEnv(const HttpRequest& request);
		void createAvAndEnv(const HttpRequest& request);
		void populateEnvVariables(const HttpRequest& request);
		void startExecution();

		std::string getScriptFileName(const HttpRequest& request) const;

	public:
		// static const std::string SCRIPT_BASE_DIR;
		// static const int TIMEOUT_SECONDS;

		CGI(const HttpRequest& request, const std::string& upload_dir, const std::string& PathToScript, const InfoServer& info);
		~CGI();

		int getFD() const { return _fd; }
		pid_t getPid() const { return _pid; }
		std::string getOutput() const { return _output; }

		// const std::string& getProcessedBody() const { return _processed_body; }
		// std::string getScriptPath() const { return _cgi_path; }
		// time_t getStartTime() const { return _start_time; }
		// pid_t getProcessId() const { return _pid; }
		// std::string getTimeoutResponse() const { return _timeout_response; }

		// void setOutputDone();
		// bool isInputDone() const { return _input_done; }
		// bool isOutputDone() const { return _output_done; }
		// bool isTimedOut() const;
		// bool hasTimedOut() const { return _timed_out; }

		// void appendOutput(int fd, const char* data, size_t len);
		// void writeInput();
		// void closePipe(int& pipe_fd);
		// void killIfTimedOut();

		// void closePipes();
};

struct CGITracker {
	CGI* cgi;              // Pointer to the CGI instance
	int pipeFd;            // Current pipe FD being polled (in_pipe, out_pipe, or err_pipe)
	int clientFd;          // Original client FD
	//const InfoServer* serverConfig; // Server-specific configuration
	std::string response;  // CGI response to be sent by Webserver
	bool isActive;
	bool firstResponseSent;
	std::string bufferedOutput; // Buffer for non-chunked responses
	bool headersParsed;         // Track if headers are parsed
	bool isChunked;             // Track if chunking is active
	std::string contentType;    // Persistent storage for parsed Content-Type
	int statusCode; // Persistent status code
	std::map<std::string, std::string> cgi_headers; // CGI headers

	// Default constructor
	CGITracker()
	: cgi(NULL), pipeFd(-1), clientFd(-1), response(""),
	isActive(false), firstResponseSent(false), bufferedOutput(""),
	headersParsed(false), isChunked(false), contentType("text/plain"), statusCode(200) {} // Initialize contentType

	// Parameterized constructor
	CGITracker(CGI* c, int pFd, int cFd)
	: cgi(c), pipeFd(pFd), clientFd(cFd), response(""),
	isActive(true), firstResponseSent(false), bufferedOutput(""),
	headersParsed(false), isChunked(false), contentType("text/plain"), statusCode(200) {} // Initialize contentType
};

#endif
