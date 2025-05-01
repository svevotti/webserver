#include "CGI.hpp"
#include "Utils.hpp"
#include <signal.h>

CGI::CGI(const HttpRequest& request, const std::string& PathToScript, const InfoServer& serverInfo)
	: _request_method(request.getMethod()),
	_cgi_path(PathToScript),
	_processed_body(request.getBodyContent()),
	_output(""),
	_av(NULL),
	_env(NULL),
	_pid(-1),
	_uri(request.getUri()),
	// _bytes_written(0),
	// _input_done(false),
	// _output_done(false),
	// _start_time(time(NULL)),
	// _timeout(serverInfo.getCGIProcessingTimeout()),
	_serverInfo(serverInfo)
{
	try
	{
		// Logger::debug("Initializing CGI for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
		// std::string content_type = request.getHttpHeaders().count("content-type") ? request.getHttpHeaders().find("content-type")->second : "";
		// Logger::debug("Content-Type: " + content_type);

		// if (_request_method == "POST")
		// 	Logger::debug("POST body size: " + Utils::toString(_processed_body.size()));
		// else if (!_processed_body.empty())
		// 	Logger::warn("Non-POST body size: " + Utils::toString(_processed_body.size()));
		// else
		// 	Logger::debug("GET request, no body");

		populateEnvVariables(request);
		createAv();
		createEnv(request);
		startExecution();
	}
	catch (const std::exception& e)
	{
		// closePipes();
		// throw CGIException("CGI init failed: " + std::string(e.what()));
	}
}

CGI::~CGI()
{
	// Free _av
	if (_av)
	{
		for (size_t i = 0; _av[i]; ++i)
			free(_av[i]);
		delete[] _av;
		_av = 0;
	}
	// Free _env
	if (_env)
	{
		for (size_t i = 0; _env[i]; ++i)
			free(_env[i]);
		delete[] _env;
		_env = 0;
	}
	_env_map.clear();
	//closePipes();
	//May be redundant because it should be cleaned after timeout or when done
	// if (_pid > 0)
	// {
	// 	kill(_pid, SIGTERM);
	// 	int status;
	// 	waitpid(_pid, &status, 0);
	// 	_pid = -1;
	// 	Logger::debug("CGI process terminated for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// }

}

void CGI::createAv()
{
	//std::string server_upload_dir = upload_to; // server-specific upload directory handling

	// if (_request_method != "POST" || server_upload_dir.empty())
	// {
	// 	_upload_to = "";
	// 	Logger::debug("No upload directory for non-POST or empty upload_to for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// }
	// else
	// {
	// 	_upload_to = server_upload_dir;
	// 	DIR* dir = opendir(_upload_to.c_str());
	// 	if (!dir)
	// 	{
	// 		Logger::debug("Creating upload directory: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// 		if (mkdir(_upload_to.c_str(), 0755) != 0)
	// 		{
	// 			Logger::error("Failed to create upload directory: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// 			throw CGIException("Failed to create upload directory: " + _upload_to);
	// 		}
	// 	}
	// 	else
	// 	{
	// 		closedir(dir);
	// 		Logger::debug("Upload directory exists: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// 	}
	// }

	_av = new char*[4];
	// Extract the file extension from cgi_path
	//Argument zero is where to find the command to run the script
	std::string extension = _cgi_path.substr(_cgi_path.find_last_of("."));
	if (_serverInfo.getCGI().locSettings["cgi_extension"].find(extension) == std::string::npos)
		throw UnsupportedMediaTypeException();
	if (extension.find(".py") != std::string::npos)
		_av[0] = strdup("/usr/bin/python3");
	else if (extension.find(".php") != std::string::npos)
		_av[0] = strdup("/usr/bin/php-cgi");
	else
		throw InternalServerErrorException();
	//Argument 1 is the script
	_av[1] = strdup(_cgi_path.c_str()); // Script/file path as argument
	//_av[2] = _upload_to.empty() ? NULL : strdup(_upload_to.c_str()); // Optional for GET
	_av[2] = NULL;
	_av[3] = NULL;

	//Logging
	std::string args = _av[2] ?
		(std::string(_av[0]) + ", " + _av[1] + ", " + _av[2]) :
		(std::string(_av[0]) + ", " + _av[1]);
	Logger::debug("CGI arguments: [" + args + "]");
}

void CGI::createEnv(const HttpRequest& request)
{
	//If we decide to pass the headers; else remove next line and headers.size()
	std::map<std::string, std::string> headers = request.getHttpHeaders();
	size_t env_size = _env_map.size() + headers.size() + 1;
	_env = new char*[env_size];
	size_t i = 0;
	std::map<std::string, std::string>::const_iterator env_it;

	Logger::debug("Creating environment variables for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

	for (env_it = _env_map.begin(); env_it != _env_map.end(); ++env_it)
	{
		std::string env_entry = env_it->first + "=" + env_it->second;
		_env[i++] = strdup(env_entry.c_str());
		//Logger::debug("ENV: " + env_entry);
	}

	//If we pass the headers
	std::map<std::string, std::string>::const_iterator header_it;
	for (header_it = headers.begin(); header_it != headers.end(); ++header_it)
	{
		std::string env_entry = "HTTP_" + header_it->first + "=" + header_it->second;
		_env[i++] = strdup(env_entry.c_str());
		//Logger::debug("ENV: " + env_entry);
	}
	_env[i] = NULL;
}

void CGI::populateEnvVariables(const HttpRequest& request)
{
	Logger::debug("Populating environment variables for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

	// Set environment variables
	_env_map["QUERY_STRING"] = request.getQuery();
	_env_map["PATH_INFO"] = "." + _serverInfo.getRoot() + _uri; // Use full filesystem path
	_env_map["REQUEST_METHOD"] = _request_method;
	_env_map["REQUEST_URI"] = _uri;// this should be the same as script_filename
	_env_map["SERVER_PROTOCOL"] = request.getProtocol();
	_env_map["SERVER_NAME"] = _serverInfo.getIP();
	_env_map["SERVER_PORT"] = _serverInfo.getPort();
	_env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env_map["REDIRECT_STATUS"] = "200"; // Required for php-cgi with force-cgi-redirect
	_env_map["SCRIPT_FILENAME"] = "." + _serverInfo.getRoot() + _uri;

	// Added now: DOCUMENT_ROOT environment variable
	_env_map["DOCUMENT_ROOT"] = _serverInfo.getRoot();
	Logger::debug("DOCUMENT_ROOT=" + _env_map["DOCUMENT_ROOT"]);

	// Set CGI processing timeout
	// _env_map["CGI_PROCESSING_TIMEOUT"] = Utils::toString(_timeout);
	// Logger::debug("CGI_PROCESSING_TIMEOUT set to: " + _env_map["CGI_PROCESSING_TIMEOUT"]);

	// Logging
	// Logger::debug("REQUEST_METHOD=" + _env_map["REQUEST_METHOD"]);
	// Logger::debug("SERVER_NAME=" + _env_map["SERVER_NAME"]);
	// Logger::debug("SERVER_PORT=" + _env_map["SERVER_PORT"]);
	// Logger::debug("SCRIPT_NAME=" + _env_map["SCRIPT_NAME"]);
	// Logger::debug("PATH_INFO=" + _env_map["PATH_INFO"]);

	// Handle POST requests
	if (_request_method == "POST")
	{
		_env_map["CONTENT_LENGTH"] = request.getContentLength();
		Logger::debug("CONTENT_LENGTH set to: " + _env_map["CONTENT_LENGTH"]);

		// Retrieve headers and set CONTENT_TYPE if available
		_env_map["CONTENT_TYPE"] = request.getContentType();
		// if (it != headers.end())
		// {
		// 	_env_map["CONTENT_TYPE"] = it->second;
		Logger::debug("CONTENT_TYPE set to: " + _env_map["CONTENT_TYPE"]);
		// }
		// else
		// 	Logger::warn("No Content-Type header for POST request; CONTENT_TYPE not set");
	}
	else // Handle GET requests
	{
		_env_map["CONTENT_LENGTH"] = "0";
		Logger::debug("Non-POST request: CONTENT_LENGTH=0");
	}
}

void CGI::startExecution()
{
	int fd_cgi[2];
	pipe(fd_cgi);
	_pid = fork();
	Logger::debug("Checking env");
	for (int i = 0; _env[i]; i++)
		Logger::debug(std::string(_env[i]));
	if (_pid == 0 ) //child
	{
		signal(SIGPIPE, SIG_IGN);// Mark SIGPIPE to prevent crashes
		signal(SIGCHLD, SIG_DFL); // Reset SIGCHLD handler in child proces
		Logger::debug("child pid: " + Utils::toString(getpid()));
		// route.path = "./"
		// this->response = retrievePage(route);
		Logger::debug("execve for " + std::string(_av[0]) + " and " + std::string(_av[1]) + " with writing FD " + Utils::toString(fd_cgi[1]));
		close(fd_cgi[0]); // Close reading
		dup2(fd_cgi[1], STDOUT_FILENO); //redirect stdout to the writing end
		close(fd_cgi[1]);
		//Currently hardcoded, need to -find which script to run; -create env for the script; -ensure that the file ext is allowed by conf_file; -check if script exist, etc.
		//const char *args[] = {"/usr/bin/python3", "script.py", NULL}; //implement env by adding to the char **, the script will call library
		execve(_av[0], _av, _env);
		perror("execv failed"); // Print error if execv fails
		// write(STDERR_FILENO, "execve failed for ", 18);
		// write(STDERR_FILENO, _av[0], strlen(_av[0]));
		// write(STDERR_FILENO, "\n", 1);
		Logger::error("execve failed for " + std::string(_av[0]) + " on Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
		exit(1); // only reached if execve fails
	}
	else if (_pid > 0)// Parent process
	{
			// Store the child PID and its corresponding file descriptor
			close(fd_cgi[1]); // Close write
			_fd = fd_cgi[0];
	}





	// Logger::debug("Starting CGI execution for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

	// if (pipe(_in_pipe) == -1 || pipe(_out_pipe) == -1 || pipe(_err_pipe) == -1)
	// {
	// 	closePipes();
	// 	throw CGIException("Failed to create pipes for Server "  + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// }
	// Logger::debug("Executing CGI: " + std::string(_av[0]) + " " + std::string(_av[1]) + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// _pid = fork();
	// if (_pid == -1)
	// {
	// 	closePipes();
	// 	throw CGIException("Failed to fork for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	// }
	// if (_pid == 0) // Child process
	// {


		// Redirect pipes
		// if (dup2(_in_pipe[0], STDIN_FILENO) == -1 ||
		// 	dup2(_out_pipe[1], STDOUT_FILENO) == -1 ||
		// 	dup2(_err_pipe[1], STDERR_FILENO) == -1)
		// {
		// 	Logger::error("Failed to redirect I/O for CGI on Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
		// 	// Close pipes and exit child process
		// 	close(_in_pipe[0]);
		// 	close(_in_pipe[1]);
		// 	close(_out_pipe[0]);
		// 	close(_out_pipe[1]);
		// 	close(_err_pipe[0]);
		// 	close(_err_pipe[1]);
		// 	exit(1);
		// }

		// Close all pipe ends in child
		// close(_in_pipe[0]);
		// close(_in_pipe[1]);
		// close(_out_pipe[0]);
		// close(_out_pipe[1]);
		// close(_err_pipe[0]);
		// close(_err_pipe[1]);

		// Execute CGI script
	// 	execve(_av[0], _av, _env);

	// }
	// else // Parent process
	// {
	// 	// Close unused pipe ends
	// 	close(_in_pipe[0]); // Close read end of input pipe
	// 	close(_out_pipe[1]); // Close write end of output pipe
	// 	close(_err_pipe[1]); // Close write end of error pipe
	// 	_in_pipe[0] = -1;
	// 	_out_pipe[1] = -1;
	// 	_err_pipe[1] = -1;

		//If nothing to send, close input pipe immpediately
		// if (_processed_body.empty())
		// {
		// 	close(_in_pipe[1]); // Close write end if no data to send
		// 	_in_pipe[1] = -1;
		// 	_input_done = true;
		// 	Logger::debug("No body to write, input done for server "  + _serverInfo.getIP() + ":" + _serverInfo.getPort() + "! Yay!");
		// }
		// Logger::debug("Parent closed unused pipes for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
	//}
}
