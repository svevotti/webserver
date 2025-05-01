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
#include <fstream>

class CGI {

	private:
		std::string _request_method;
		std::string _upload_to;
		std::string _cgi_path;
		std::string _processed_body;
		std::string _output;
		char** _av;
		char** _env;
		int fd;
		pid_t _pid;
		const InfoServer& _serverInfo;
		std::map<std::string, std::string> _env_map;
		std::string	_uri;
		// size_t _bytes_written;
		// bool _input_done;
		// bool _output_done;
		// time_t _start_time;
		// double _timeout;
		// std::string _error_output; // Store STDERR output
		// bool _timed_out; // Track timeout state
		// std::string _timeout_response; // Store timeout response

		void createAv(const std::string& cgi_path, const std::string& upload_to);
		void createEnv(const HttpRequest& request);
		void createAvAndEnv(const HttpRequest& request);
		void populateEnvVariables(const HttpRequest& request);
		void startExecution();

		std::string getScriptFileName(const HttpRequest& request) const;

	public:
		// static const std::string SCRIPT_BASE_DIR;
		// static const int TIMEOUT_SECONDS;

		CGI(const HttpRequest& request, const std::string& upload_to, const InfoServer& info);
		~CGI();

		int getFD() const { return fd; }
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


#endif
