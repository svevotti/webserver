#include "CGI.hpp"
#include "Logger.hpp"

const std::string CGI::SCRIPT_BASE_DIR = "./cgi_bin/";
const int CGI::TIMEOUT_SECONDS = 20;

CGI::CGI(const ClientRequest& request, const std::string& upload_to, const InfoServer& serverInfo, const std::string& raw_body)
    : _request_method(request.getRequestLine().find("Method")->second),
      _upload_to(upload_to),
      _processed_body(raw_body),
      _output(""),
      _av(NULL),
      _env(NULL),
      _in_pipe(),
      _out_pipe(),
      _pid(-1),
      _stderr_fd(-1),
      _bytes_written(0),
      _input_done(false),
      _output_done(false),
      _start_time(time(NULL)),
      _serverInfo(serverInfo)
{
    _in_pipe[0] = -1; _in_pipe[1] = -1;
    _out_pipe[0] = -1; _out_pipe[1] = -1;

    try 
    {
        populateEnvVariables(request);
        _cgi_path = getScriptFileName(request);
        createAvAndEnv(request);
        std::ostringstream oss;
        oss << _processed_body.size();
        Logger::debug("Raw body size: " + oss.str());
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
        for (size_t i = 0; _av[i]; ++i) free(_av[i]);
        delete[] _av;
        _av = NULL;
    }
    if (_env) 
    {
        for (size_t i = 0; _env[i]; ++i) free(_env[i]);
        delete[] _env;
        _env = NULL;
    }
    closePipes();
    closePipe(_stderr_fd); // Close stderr FD
    if (_pid > 0) 
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
        _pid = -1;
    }
}

std::string CGI::getScriptFileName(const ClientRequest& request) const 
{
    std::string uri = request.getRequestLine().find("Request-URI")->second;
    std::string cgi_root = SCRIPT_BASE_DIR;
    if (uri == "/upload") 
    {
        std::string script_path = cgi_root + "upload.py";
        if (access(script_path.c_str(), F_OK) != 0)
            throw CGIException("Script file does not exist: " + script_path);
        return script_path;
    }
    size_t pos = uri.find_last_of('/');
    std::string script_name = (pos != std::string::npos) ? uri.substr(pos + 1) : uri;
    size_t qpos = script_name.find('?');
    if (qpos != std::string::npos) script_name = script_name.substr(0, qpos);
    if (script_name.empty() || script_name.find('.') == std::string::npos || script_name.substr(script_name.find_last_of('.')) != ".py")
        throw CGIException("Invalid script extension. Only .py scripts are supported.");
    std::string full_path = cgi_root + script_name;
    if (access(full_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + full_path);
    return full_path;
}

void CGI::createAv(const std::string& cgi_path, const std::string& upload_to) 
{
    if (upload_to.empty())
    {
        Logger::warn("Upload directory is empty, defaulting to './uploads'");
        _upload_to = "./uploads"; // Default directory
    }
    if (!opendir(_upload_to.c_str()))
    {
        Logger::debug("Creating upload directory: " + _upload_to);
        mkdir(_upload_to.c_str(), 0755);
    }
    _av = new char*[4];
    _av[0] = strdup("/usr/bin/python3");
    _av[1] = strdup(cgi_path.c_str());
    _av[2] = strdup(upload_to.c_str());
    _av[3] = NULL;
    Logger::debug("CGI arguments: [" + std::string(_av[0]) + ", " + _av[1] + ", " + _av[2] + "]");
}

void CGI::createEnv(const ClientRequest& request) 
{
    std::map<std::string, std::string> headers = request.getHeaders();
    std::map<std::string, std::string> queryMap = request.getUriQueryMap();

    size_t env_size = _env_map.size() + headers.size() + queryMap.size();
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
    for (std::map<std::string, std::string>::const_iterator it = queryMap.begin(); it != queryMap.end(); ++it) 
    {
        std::string env_entry = "QUERY_" + it->first + "=" + it->second;
        _env[i++] = strdup(env_entry.c_str());
    }
    _env[i] = NULL;
}

void CGI::createAvAndEnv(const ClientRequest& request) 
{
    if (access(_cgi_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + _cgi_path);
    createAv(_cgi_path, _upload_to);
    createEnv(request);
}

bool CGI::unchunkRequest(const std::string& chunked, std::string& unchunked) 
{
    std::istringstream iss(chunked);
    std::string line;
    long chunk_size;
    while (std::getline(iss, line)) 
    {
        if (line.empty()) continue;
        char* end_ptr;
        chunk_size = strtol(line.c_str(), &end_ptr, 16);
        if (*end_ptr != '\0' || chunk_size < 0) throw CGIException("Invalid chunk size");
        if (chunk_size == 0) break;
        std::vector<char> buffer(chunk_size);
        iss.read(&buffer[0], chunk_size);
        if (iss.gcount() != static_cast<std::streamsize>(chunk_size))
            throw CGIException("Failed to read chunk data");
        unchunked.append(&buffer[0], chunk_size);
    }
    return !unchunked.empty();
}

void CGI::populateEnvVariables(const ClientRequest& request) 
{
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
    _env_map["REQUEST_URI"] = uri;
    _env_map["SERVER_PROTOCOL"] = requestLine.find("Protocol")->second;
    _env_map["SERVER_NAME"] = "127.0.0.1";
    std::vector<std::string> ports = _serverInfo.getArrayPorts();
    _env_map["SERVER_PORT"] = ports.empty() ? "8080" : ports[0];
    _env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
    if (_request_method == "POST") 
    {
        std::ostringstream oss;
        oss << _processed_body.size();
        _env_map["CONTENT_LENGTH"] = oss.str();
        std::map<std::string, std::string> headers = request.getHeaders();
        std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
        if (it != headers.end()) 
        {
            _env_map["CONTENT_TYPE"] = it->second;
            Logger::debug("CGI CONTENT_TYPE set to: " + it->second);
        }
        it = headers.find("Transfer-Encoding");
        if (it != headers.end() && it->second == "chunked") 
        {
            std::string unchunked;
            if (unchunkRequest(_processed_body, unchunked)) 
            {
                _processed_body = unchunked;
                oss.str(""); oss << _processed_body.size();
                _env_map["CONTENT_LENGTH"] = oss.str();
            }
        }
    } 
    else
        _env_map["CONTENT_LENGTH"] = "0";
}

void CGI::startExecution()
{
    int stderr_pipe[2];
    Logger::debug("Creating pipes for CGI");
    if (pipe(_in_pipe) == -1 || pipe(_out_pipe) == -1 || pipe(stderr_pipe) == -1)
    {
        closePipes();
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        throw CGIException("Failed to create pipes");
    }
    Logger::debug("Forking CGI process");
    _pid = fork();
    if (_pid == -1)
    {
        closePipes();
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        throw CGIException("Failed to fork");
    }
    if (_pid == 0) // Child
    {
        close(_in_pipe[1]); close(_out_pipe[0]); close(stderr_pipe[0]);
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 ||
            dup2(_out_pipe[1], STDOUT_FILENO) == -1 ||
            dup2(stderr_pipe[1], STDERR_FILENO) == -1)
            exit(1);
        close(_in_pipe[0]); close(_out_pipe[1]); close(stderr_pipe[1]);
        execve(_av[0], _av, _env);
        exit(1);
    }
    else // Parent
    {
        Logger::debug("In parent, closing unused pipe ends");
        close(_in_pipe[0]); close(_out_pipe[1]); close(stderr_pipe[1]);
        _out_pipe[1] = -1;
        if (_processed_body.empty())
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("No body to write, input done");
        }
        _stderr_fd = stderr_pipe[0]; // Store for polling
        std::ostringstream oss;
        oss << "CGI stderr FD stored for polling: " << _stderr_fd;
        Logger::debug(oss.str());
    }
}

void CGI::appendOutput(const char* data, size_t len) 
{
    _output.append(data, len);
}

void CGI::writeInput()
{
    Logger::debug("Entering writeInput");
    if (_input_done || _in_pipe[1] == -1)
    {
        Logger::debug("CGI writeInput skipped: already done or pipe closed");
        return;
    }
    size_t remaining = _processed_body.size() - _bytes_written;
    if (remaining <= 0)
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        Logger::debug("CGI input completed (no remaining bytes)");
        return;
    }
    const size_t CHUNK_SIZE = 1024;
    size_t to_write = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
    std::ostringstream oss;
    oss << "Writing " << to_write << " bytes to CGI";
    Logger::debug(oss.str());
    ssize_t written = write(_in_pipe[1], _processed_body.c_str() + _bytes_written, to_write);
    if (written > 0)
    {
        _bytes_written += written;
        oss.str("");
        oss << "Wrote " << written << " of " << remaining << " bytes to CGI";
        Logger::debug(oss.str());
        if (_bytes_written >= _processed_body.size())
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("CGI input fully written and pipe closed");
        }
    }
    else
    {
        oss.str("");
        oss << "CGI write failed or pipe closed, written: " << (written == 0 ? "0" : "-1");
        Logger::debug(oss.str());
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
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
    closePipe(_in_pipe[0]);
    closePipe(_in_pipe[1]);
    closePipe(_out_pipe[0]);
    closePipe(_out_pipe[1]);
}

bool CGI::isTimedOut() const 
{
    return time(NULL) - _start_time >= TIMEOUT_SECONDS;
}

void CGI::killIfTimedOut() 
{
    if (isTimedOut() && _pid > 0) 
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
        _pid = -1;
        _input_done = true;
        _output_done = true;
        closePipes();
        Logger::warn("CGI process killed due to timeout");
    }
}