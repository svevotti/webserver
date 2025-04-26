#include "CGI.hpp"
#include "Utils.hpp" 
#include <signal.h>

const std::string CGI::SCRIPT_BASE_DIR = "./www/cgi-bin/";

CGI::CGI(const HttpRequest& request, const std::string& upload_to, const InfoServer& serverInfo)
    : _request_method(request.getHttpRequestLine()["method"]),
      _upload_to(upload_to),
      _cgi_path(""),
      _processed_body(request.getBodyContent()),
      _output(""),
      _av(NULL),
      _env(NULL),
      _pid(-1),
      _bytes_written(0),
      _input_done(false),
      _output_done(false),
      _start_time(time(NULL)),
      _timeout(serverInfo.getCGIProcessingTimeout()),
      _serverInfo(serverInfo)
{
    // Initialize pipe FDs to invalid values
    _in_pipe[0] = -1; _in_pipe[1] = -1;
    _out_pipe[0] = -1; _out_pipe[1] = -1;
    _err_pipe[0] = -1; _err_pipe[1] = -1;

    try 
    {
        Logger::debug("Initializing CGI for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        std::string content_type = request.getHttpHeaders().count("content-type") ? request.getHttpHeaders().find("content-type")->second : "";
        Logger::debug("Content-Type: " + content_type);

        if (_request_method == "POST")
            Logger::debug("POST body size: " + Utils::toString(_processed_body.size()));
        else if (!_processed_body.empty())
            Logger::warn("Non-POST body size: " + Utils::toString(_processed_body.size()));
        else
            Logger::debug("GET request, no body");

        populateEnvVariables(request);
        _cgi_path = getScriptFileName(request);
        createAvAndEnv(request);
        startExecution();
    } 
    catch (const std::exception& e)
    {       
        closePipes();
        throw CGIException("CGI init failed: " + std::string(e.what()));
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
    closePipes();
    if (_pid > 0) 
    {
        kill(_pid, SIGTERM);
        int status;
        waitpid(_pid, &status, 0);
        _pid = -1;
        Logger::debug("CGI process terminated for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    }
    
}

void CGI::setOutputDone() 
{
    _output_done = true;
    Logger::info("Marked CGI output as done for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
}

std::string CGI::getScriptFileName(const HttpRequest& request) const 
{
    std::string uri = request.getHttpRequestLine()["request-uri"];
    
    // Extract server-specific CGI path
    std::string cgi_root = _serverInfo.getCGI().path;
 
    // Validation for CGI path
    if (cgi_root.empty())
    {
        Logger::error("CGI path is empty for server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        throw CGIException("CGI path is not configured for this server");
    }

    Logger::debug("Constructed CGI root: " + cgi_root + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

    // Extract the script name from the uri
    size_t pos = uri.find_last_of('/');
    std::string script_name = (pos != std::string::npos && pos + 1 < uri.length()) ? uri.substr(pos + 1) : uri;

    // Remove query parameters (if any)
    size_t qpos = script_name.find('?');
    if (qpos != std::string::npos) 
        script_name = script_name.substr(0, qpos);
    
    // Validate script name is not empty and has an extension
    if (script_name.empty() || script_name.find('.') == std::string::npos)
        throw CGIException("Invalid script name or missing extension: " + script_name);

    // Extract and validate the file extension
    std::string extension = script_name.substr(script_name.find_last_of('.'));
    if (extension != ".py" && extension != ".php")
        throw CGIException("Unsupported script extension: " + extension);
        
    // Construct the full path to the script
    std::string full_path = cgi_root;
    if (full_path[full_path.length() - 1] != '/')
        full_path += "/";
    full_path += script_name;
    
    // Check if the script exists
    if (access(full_path.c_str(), F_OK) != 0)
    {
        Logger::error("Script file does not exist: " + full_path + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        throw CGIException("Script file does not exist: " + full_path);
    }
    Logger::debug("Determined full CGI script path: " + full_path + " for server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    return full_path;
}

void CGI::createAv(const std::string& cgi_path, const std::string& upload_to) 
{
    std::string server_upload_dir = upload_to; // server-specific upload directory handling

    if (_request_method != "POST" || server_upload_dir.empty()) 
    {
        _upload_to = "";
        Logger::debug("No upload directory for non-POST or empty upload_to for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    } 
    else
    {
        _upload_to = server_upload_dir;
        DIR* dir = opendir(_upload_to.c_str());
        if (!dir)
        {
            Logger::debug("Creating upload directory: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
            if (mkdir(_upload_to.c_str(), 0755) != 0) 
            {
                Logger::error("Failed to create upload directory: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
                throw CGIException("Failed to create upload directory: " + _upload_to);
            }
        }
        else
        {
            closedir(dir);
            Logger::debug("Upload directory exists: " + _upload_to + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        }
    }

    _av = new char*[4];

    // Extract the file extension from cgi_path
    std::string extension = cgi_path.substr(cgi_path.find_last_of("."));
    if (extension == ".py")
        _av[0] = strdup("/usr/bin/python3");
    else if (extension == ".php")
        _av[0] = strdup("/usr/bin/php-cgi");
    else
        throw CGIException("Unsupported interpreter for extension: " + extension);

    _av[1] = strdup(cgi_path.c_str()); // Script/file path as argument
    _av[2] = _upload_to.empty() ? NULL : strdup(_upload_to.c_str()); // Optional for GET
    _av[3] = NULL;

    //Logging
    std::string args = _av[2] ? 
        (std::string(_av[0]) + ", " + _av[1] + ", " + _av[2]) : 
        (std::string(_av[0]) + ", " + _av[1]);
    Logger::debug("CGI arguments: [" + args + "]");
}

void CGI::createEnv(const HttpRequest& request) 
{
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
        Logger::debug("ENV: " + env_entry); 
    }

    std::map<std::string, std::string>::const_iterator header_it;
    for (header_it = headers.begin(); header_it != headers.end(); ++header_it) 
    {
        std::string env_entry = "HTTP_" + header_it->first + "=" + header_it->second;
        _env[i++] = strdup(env_entry.c_str());
        Logger::debug("ENV: " + env_entry); 
    }
    _env[i] = NULL;
}

void CGI::createAvAndEnv(const HttpRequest& request) 
{
    if (access(_cgi_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + _cgi_path + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    createAv(_cgi_path, _upload_to);
    createEnv(request);
}

void CGI::populateEnvVariables(const HttpRequest& request) 
{
    Logger::debug("Populating environment variables for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

    // Set environment variables
    std::string uri = request.getHttpRequestLine()["request-uri"];
    size_t qpos = uri.find('?');
    _env_map["QUERY_STRING"] = qpos != std::string::npos ? uri.substr(qpos + 1) : "";
    _env_map["PATH_INFO"] = getScriptFileName(request); // Use full filesystem path
    _env_map["REQUEST_METHOD"] = _request_method;
    _env_map["REQUEST_URI"] = uri;
    _env_map["SERVER_PROTOCOL"] = request.getHttpRequestLine()["protocol"];
    _env_map["SERVER_NAME"] = _serverInfo.getIP();
    _env_map["SERVER_PORT"] = _serverInfo.getPort();
    _env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env_map["REDIRECT_STATUS"] = "200"; // Required for php-cgi with force-cgi-redirect
    _env_map["SCRIPT_FILENAME"] = getScriptFileName(request);

    // Added now: DOCUMENT_ROOT environment variable
    _env_map["DOCUMENT_ROOT"] = _serverInfo.getRoot();
    Logger::debug("DOCUMENT_ROOT=" + _env_map["DOCUMENT_ROOT"]);
    
    // Set CGI processing timeout
    _env_map["CGI_PROCESSING_TIMEOUT"] = Utils::toString(_timeout);
    Logger::debug("CGI_PROCESSING_TIMEOUT set to: " + _env_map["CGI_PROCESSING_TIMEOUT"]);

    // Logging
    Logger::debug("SERVER_NAME=" + _env_map["SERVER_NAME"]);
    Logger::debug("SERVER_PORT=" + _env_map["SERVER_PORT"]);
    Logger::debug("SCRIPT_FILENAME=" + _env_map["SCRIPT_FILENAME"]);
    Logger::debug("PATH_INFO=" + _env_map["PATH_INFO"]);

    // Handle POST requests
    if (_request_method == "POST")
    {
        _env_map["CONTENT_LENGTH"] = Utils::toString(_processed_body.size());
        Logger::debug("CONTENT_LENGTH set to: " + _env_map["CONTENT_LENGTH"]);

        // Retrieve headers and set CONTENT_TYPE if available
        std::map<std::string, std::string> headers = request.getHttpHeaders();
        std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
        if (it != headers.end()) 
        {
            _env_map["CONTENT_TYPE"] = it->second;
            Logger::debug("CONTENT_TYPE set to: " + _env_map["CONTENT_TYPE"]);
        }
        else
            Logger::warn("No Content-Type header for POST request; CONTENT_TYPE not set");
    } 
    else // Handle GET requests 
    {
        _env_map["CONTENT_LENGTH"] = "0";
        Logger::debug("Non-POST request: CONTENT_LENGTH=0");
    }
}

void CGI::startExecution()
{
    Logger::debug("Starting CGI execution for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());

    if (pipe(_in_pipe) == -1 || pipe(_out_pipe) == -1 || pipe(_err_pipe) == -1) 
    {
        closePipes();
        throw CGIException("Failed to create pipes for Server "  + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    }
    Logger::debug("Executing CGI: " + std::string(_av[0]) + " " + std::string(_av[1]) + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    _pid = fork();
    if (_pid == -1) 
    {
        closePipes();
        throw CGIException("Failed to fork for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    }
    if (_pid == 0) // Child process
    {
        signal(SIGPIPE, SIG_IGN);// Mark SIGPIPE to prevent crashes
        signal(SIGCHLD, SIG_DFL); // Reset SIGCHLD handler in child proces
      
        // Redirect pipes
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 ||
            dup2(_out_pipe[1], STDOUT_FILENO) == -1 ||
            dup2(_err_pipe[1], STDERR_FILENO) == -1) 
        {
            Logger::error("Failed to redirect I/O for CGI on Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
            // Close pipes and exit child process
            close(_in_pipe[0]);
            close(_in_pipe[1]);
            close(_out_pipe[0]);
            close(_out_pipe[1]);
            close(_err_pipe[0]);
            close(_err_pipe[1]);
            exit(1);
        }

        // Close all pipe ends in child
        close(_in_pipe[0]);
        close(_in_pipe[1]);
        close(_out_pipe[0]);
        close(_out_pipe[1]);
        close(_err_pipe[0]);
        close(_err_pipe[1]);
        
        // Execute CGI script
        execve(_av[0], _av, _env);
        write(STDERR_FILENO, "execve failed for ", 18);
        write(STDERR_FILENO, _av[0], strlen(_av[0]));
        write(STDERR_FILENO, "\n", 1);
        Logger::error("execve failed for " + std::string(_av[0]) + " on Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        exit(1); // only reached if execve fails
    } 
    else // Parent process
    {
        // Close unused pipe ends
        close(_in_pipe[0]); // Close read end of input pipe
        close(_out_pipe[1]); // Close write end of output pipe
        close(_err_pipe[1]); // Close write end of error pipe
        _in_pipe[0] = -1;
        _out_pipe[1] = -1;
        _err_pipe[1] = -1;
   
        //If nothing to send, close input pipe immpediately
        if (_processed_body.empty())
        {
            close(_in_pipe[1]); // Close write end if no data to send
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("No body to write, input done for server "  + _serverInfo.getIP() + ":" + _serverInfo.getPort() + "! Yay!");
        }
        Logger::debug("Parent closed unused pipes for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    }
}

void CGI::appendOutput(int fd, const char* data, size_t len) 
{
    if (fd == _out_pipe[0]) 
    {
        _output.append(data, len);
        Logger::debug("Appended " + Utils::toString(len) + " bytes from stdout to CGI output for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    } 
    else if (fd == _err_pipe[0]) 
        Logger::debug("CGI stderr: " + std::string(data, len) + " for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    else 
        Logger::warn("Unexpected fd " + Utils::toString(fd) + " in appendOutput for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
}

void CGI::writeInput() 
{
    // Skip writing for non-POST requests
    if (_request_method != "POST" || _processed_body.empty())
    {
        Logger::debug("No input to write for Server "  + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        return;
    }

    // Skip if already done or pipe closed
    if (_input_done || _in_pipe[1] == -1) 
    {
        Logger::debug("CGI writeInput skipped: already done or pipe closed for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort() + ". We good!");
        return;
    }

    // Check if all data has been written
    size_t remaining = _processed_body.size() - _bytes_written;
    if (remaining <= 0) 
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        Logger::debug("Completed writing input to CGI for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        return;
    }

    // Write a chunk of data
    const size_t CHUNK_SIZE = 1024;
    size_t to_write = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
    ssize_t written = write(_in_pipe[1], _processed_body.c_str() + _bytes_written, to_write);
    
    // Handle write result (without errno)
    if (written > 0) 
    {
        _bytes_written += written;
        Logger::debug("Wrote " + Utils::toString(written) + " bytes to CGI script for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        
        // Check if all data has been written
        if (_bytes_written >= _processed_body.size()) 
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("Finished writing CGI input and closed pipe. Let's go.");
        }
    } 
    else 
    {
        if (written == 0)
            Logger::debug("CGI write returned 0 bytes. We'll retry later for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
        else // Error occured
        {
            //Close pipe and mark as done.
            Logger::warn("CGI write failed or pipe closed unexpectedly for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort() + ". Yikes! Closing pipe");
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
        }
    }
}

void CGI::closePipe(int& pipe_fd) 
{
    if (pipe_fd != -1) 
    {
        close(pipe_fd);
        pipe_fd = -1;
    }
}

void CGI::closePipes() 
{
    Logger::debug("Closing all pipes for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort());
    closePipe(_in_pipe[0]);
    closePipe(_in_pipe[1]);
    closePipe(_out_pipe[0]);
    closePipe(_out_pipe[1]);
    closePipe(_err_pipe[0]);
    closePipe(_err_pipe[1]);
}

bool CGI::isTimedOut() const 
{
    time_t currentTime = time(NULL);
    time_t elapsed = currentTime - _start_time;
    Logger::debug("isTimedOut for script " + _cgi_path + ": currentTime=" + Utils::toString(currentTime) + 
                 ", start_time=" + Utils::toString(_start_time) + 
                 ", elapsed=" + Utils::toString(elapsed) + 
                 ", timeout=" + Utils::toString(_timeout));
    return elapsed >= _timeout;
}

void CGI::killIfTimedOut() 
{
    if (isTimedOut() && _pid > 0) 
    {
        Logger::debug("Sending SIGTERM to CGI PID " + Utils::toString(_pid) + " for script " + _cgi_path);
        kill(_pid, SIGTERM);
        int status;
        waitpid(_pid, &status, 0);
        _pid = -1;
        _input_done = true;
        _output_done = true;
        closePipes();
        Logger::warn("CGI process terminated due to timeout for Server " + _serverInfo.getIP() + ":" + _serverInfo.getPort() + ". What a sad death :(");
    }
}
