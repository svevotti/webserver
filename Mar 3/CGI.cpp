#include "CGI.hpp"

const std::string CGI::SCRIPT_BASE_DIR = "./cgi_bin/";
const int CGI::TIMEOUT_SECONDS = 10;

CGI::CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& info)
    : _request_method(request.getRequestLine().find("Method")->second), 
      _upload_to(upload_to),
      _processed_body(""), 
      _output(""),
      _av(NULL), 
      _env(NULL),
      _pid(-1), 
      _bytes_written(0), 
      _input_done(false), 
      _output_done(false), 
      _start_time(time(NULL))
{
    _in_pipe[0] = -1; _in_pipe[1] = -1;
    _out_pipe[0] = -1; _out_pipe[1] = -1;

    try 
    {
        populateEnvVariables(request, info);
        _cgi_path = getScriptFileName(request, info);
        createAvAndEnv(request, info);
        processRequestBody(request);
        startExecution();
    } 
    catch (const std::exception& e) 
    {
        cleanup();
        throw;
    }
}

CGI::~CGI()
{
    cleanup();
}

void CGI::cleanup()
{
    if (_av) 
    {
        for (size_t i = 0; _av[i]; ++i)
            free(_av[i]);
        delete[] _av;
        _av = NULL;
    }
    if (_env) 
    {
        for (size_t i = 0; _env[i]; ++i)
            free(_env[i]);
        delete[] _env;
        _env = NULL;
    }
    closePipes();
    if (_pid > 0) 
    {
        kill(_pid, SIGKILL); // Ensure the process is terminated
        waitpid(_pid, NULL, 0); // Reap the process
        _pid = -1;
    }
}

std::string CGI::getScriptFileName(const ClientRequest& request, const InfoServer& info) const
{
    (void)info; // Suppress unused parameter warning
    std::string uri = request.getRequestLine().find("Request-URI")->second;
    size_t pos = uri.find_last_of('/');
    std::string script_name = (pos != std::string::npos) ? uri.substr(pos + 1) : uri;
    size_t qpos = script_name.find('?');
    if (qpos != std::string::npos)
        script_name = script_name.substr(0, qpos);

    if (script_name.empty() || script_name.substr(script_name.find_last_of('.')) != ".py")
        throw CGIException("Invalid script extension. Only .py scripts are supported.");

    std::string cgi_root = "./cgi_bin/";
    if (script_name[0] != '/')
        script_name = cgi_root + script_name;
    else if (script_name.find(cgi_root) != 0)
        throw CGIException("Script path must be under " + cgi_root);

    return script_name;
}


void CGI::createAv(const std::string& cgi_path, const std::string& upload_to) 
{
    _av = new char*[4];
    _av[0] = strdup("/usr/bin/python3");
    _av[1] = strdup(cgi_path.c_str());
    _av[2] = strdup(upload_to.c_str());
    _av[3] = NULL;
}

void CGI::createEnv(const ClientRequest& request, const InfoServer& info) 
{
    (void)info;
    std::map<std::string, std::string> requestLine = request.getRequestLine();
    std::map<std::string, std::string> headers = request.getHeaders();
    std::map<std::string, std::string> query = request.getUriQueryMap();
    size_t env_size = _env_map.size() + headers.size() + query.size();
    _env = new char*[env_size + 1];
    size_t i = 0;

    for (std::map<std::string, std::string>::const_iterator it = _env_map.begin(); it != _env_map.end(); ++it) 
    {
        std::string env_entry = it->first + "=" + it->second;
        _env[i++] = strdup(env_entry.c_str());
    }

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) 
    {
        std::string env_entry = "HTTP_" + it->first + "=" + it->second;
        _env[i++] = strdup(env_entry.c_str());
    }

    for (std::map<std::string, std::string>::const_iterator it = query.begin(); it != query.end(); ++it) 
    {
        std::string env_entry = "QUERY_" + it->first + "=" + it->second;
        _env[i++] = strdup(env_entry.c_str());
    }
    _env[i] = NULL;
}

// Create argument vector and environment
void CGI::createAvAndEnv(const ClientRequest& request, const InfoServer& info) 
{
    if (access(_cgi_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + _cgi_path);

    createAv(_cgi_path, _upload_to);
    createEnv(request, info);
}

//UnchunkRequest
bool CGI::unchunkRequest(const std::string& chunked, std::string& unchunked) 
{
    std::istringstream iss(chunked);
    std::string line;
    long chunk_size;

    while (std::getline(iss, line)) 
    {
        if (line.empty())
            continue;

        // Validate hexadecimal chunk size
        for (std::string::const_iterator it = line.begin(); it != line.end(); ++it) 
        {
            if (!isxdigit(*it))
                throw CGIException("Invalid hexadecimal string in chunked request.");
        }

        // Convert chunk size from hex to long
        char* end_ptr;
        errno = 0;
        chunk_size = strtol(line.c_str(), &end_ptr, 16);

        // Check for conversion errors
        if ((errno == ERANGE && (chunk_size == LONG_MAX || chunk_size == LONG_MIN)) ||
            (end_ptr == line.c_str()) || (*end_ptr != '\0'))
            throw CGIException("Invalid chunk size in chunked request.");

        if (chunk_size == 0)
            break;

        // Read chunk data
        std::vector<char> buffer(chunk_size);
        iss.read(&buffer[0], chunk_size);
        if (iss.gcount() != static_cast<std::streamsize>(chunk_size))
            throw CGIException("Failed to read chunk data.");

        unchunked.append(&buffer[0], chunk_size);
    }
    return !unchunked.empty();
}

void CGI::processRequestBody(const ClientRequest& request)
{
    if (_request_method == "GET") 
        _processed_body = ""; 
    else if (_request_method == "POST") 
    {
        std::map<std::string, std::string> headers = request.getHeaders();
        std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
        if (it != headers.end() && it->second == "chunked") 
        {
            if (!unchunkRequest(request.getBodyText(), _processed_body))
                throw CGIException("Failed to unchunk POST request body.");
        } 
        else 
            _processed_body = request.getBodyText();
    } 
    else 
        throw CGIException("Unsupported REQUEST_METHOD: " + _request_method);
}

void CGI::populateEnvVariables(const ClientRequest& request, const InfoServer& info)
{
    (void)info; // Suppress unused parameter warning
    std::map<std::string, std::string> requestLine = request.getRequestLine();
    std::string uri = requestLine.find("Request-URI")->second;
    size_t qpos = uri.find('?');
    if (qpos != std::string::npos) 
    {
        _env_map["QUERY_STRING"] = uri.substr(qpos + 1);
        _env_map["PATH_INFO"] = uri.substr(0, qpos);
    } 
    else 
    {
        _env_map["QUERY_STRING"] = "";
        _env_map["PATH_INFO"] = uri;
    }
    _env_map["REQUEST_METHOD"] = _request_method;
    _env_map["REQUEST_URI"] = requestLine.find("Request-URI")->second;
    _env_map["SERVER_PROTOCOL"] = requestLine.find("Protocol")->second;
    _env_map["SERVER_NAME"] = "127.0.0.1";
    _env_map["SERVER_PORT"] = "8080"; // use info.getPort() later
    _env_map["GATEWAY_INTERFACE"] = "CGI/1.1";

    std::ostringstream oss;
    if (_request_method == "POST") 
    {
        oss << _processed_body.size();
        _env_map["CONTENT_LENGTH"] = oss.str();
    } 
    else
    {
        _env_map["CONTENT_LENGTH"] = "0";
    }
}

void CGI::startExecution()
{
    if (pipe(_in_pipe) == -1)
        throw CGIException("Failed to create input pipe");
    
    if (pipe(_out_pipe) == -1) 
    {
        close(_in_pipe[0]);
        close(_in_pipe[1]);
        throw CGIException("Failed to create output pipe");
    }

    _pid = fork();
    if (_pid == -1) 
    {
        close(_in_pipe[0]);
        close(_in_pipe[1]);
        close(_out_pipe[0]);
        close(_out_pipe[1]);
        throw CGIException("Failed to fork");
    }

    if (_pid == 0) // child (CGI)
    {
        close(_in_pipe[1]);
        close(_out_pipe[0]);
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 || dup2(_out_pipe[1], STDOUT_FILENO) == -1) 
            exit(1);
        close(_in_pipe[0]);
        close(_out_pipe[1]);
        if (execve(_av[0], _av, _env) == -1) 
        {
            std::cerr << "Error: execve() failed." << std::endl;
            exit(1);
        }
    } 
    else // parent (Server)
    {
        close(_in_pipe[0]);
        close(_out_pipe[1]);
        _out_pipe[1] = -1;

        if (_processed_body.empty()) 
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
        }
        // Input writing deferred to ServerSocket's poll loop
    }
}

void CGI::appendOutput(const char* data, size_t len)
{
    _output.append(data, len);
}

void CGI::updateBytesWritten(size_t bytes)
{
    _bytes_written += bytes;
    if (_bytes_written >= _processed_body.size()) 
    {
        _input_done = true;
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
    }
}

bool CGI::isDone() const
{
    if (_pid <= 0) return true;
    return _input_done && _output_done;
}

bool CGI::isTimedOut() const
{
    return (time(NULL) - _start_time >= TIMEOUT_SECONDS);
}

// Close a pipe
void CGI::closePipe(int pipe_fd) 
{
    if (pipe_fd != -1) 
    {
        close(pipe_fd);
        pipe_fd = -1;
    }
}

// Close all pipes
void CGI::closePipes() 
{
    closePipe(_in_pipe[1]);
    closePipe(_out_pipe[0]);
}


// Write input to CGI
void CGI::writeInput(const std::string& data, int fd) 
{
    size_t remaining = data.size() - _bytes_written;
    if (remaining <= 0) 
    {
        if (!_input_done) 
        {
            close(fd);
            _input_done = true;
        }
        return;
    }

    ssize_t written = write(fd, data.c_str() + _bytes_written, remaining);
    if (written > 0) 
    {
        updateBytesWritten(written);
        std::cout << "Wrote " << written << " bytes to CGI (PID " << _pid << ")" << std::endl;
    } 
    else if (written <= 0) 
    {
        // Treat any non-positive return value as "done" or "failed"
        updateBytesWritten(remaining);  // Force completion
        std::cout << "CGI input write finished or failed (PID " << _pid << ")" << std::endl;
    }
}

