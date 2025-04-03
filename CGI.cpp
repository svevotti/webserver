#include "CGI.hpp"
#include "Utils.hpp" 

const std::string CGI::SCRIPT_BASE_DIR = "./cgi_bin/";
const int CGI::TIMEOUT_SECONDS = 20;

CGI::CGI(const HttpRequest& request, const std::string& upload_to, const InfoServer& serverInfo, const std::string& raw_body)
    : _request_method(request.getHttpRequestLine()["method"]),
      _upload_to(upload_to),
      _processed_body(raw_body),
      _output(""),
      _av(NULL),
      _env(NULL),
      _pid(-1),
      _bytes_written(0),
      _input_done(false),
      _output_done(false),
      _start_time(time(NULL)),
      _serverInfo(serverInfo)
{
    _in_pipe[0] = -1; _in_pipe[1] = -1;
    _out_pipe[0] = -1; _out_pipe[1] = -1;
    _err_pipe[0] = -1; _err_pipe[1] = -1;

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
        closePipes();
        throw CGIException("CGI initialization failed: " + std::string(e.what()));
    }
}

CGI::~CGI() 
{
    if (_av) 
    {
        for (size_t i = 0; _av[i]; ++i) free(_av[i]);
        delete[] _av;
    }
    if (_env) 
    {
        for (size_t i = 0; _env[i]; ++i) free(_env[i]);
        delete[] _env;
    }
    closePipes();
    if (_pid > 0) 
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
    }
}

void CGI::setOutputDone() 
{
    _output_done = true;
}

std::string CGI::getScriptFileName(const HttpRequest& request) const 
{
    std::string uri = request.getHttpRequestLine()["request-uri"];
    std::string cgi_root = "./www/cgi-bin/"; // Relative to project root where ./webserver runs
    Logger::debug("CGI root: " + cgi_root);

    if (uri == "/upload") 
    {
        std::string script_path = cgi_root + "upload.py";
        Logger::debug("Upload script path: " + script_path);
        if (access(script_path.c_str(), F_OK) != 0)
            throw CGIException("Script file does not exist: " + script_path);
        return script_path;
    }

    size_t pos = uri.find_last_of('/');
    std::string script_name = (pos != std::string::npos) ? uri.substr(pos + 1) : uri;
    size_t qpos = script_name.find('?');
    if (qpos != std::string::npos) 
        script_name = script_name.substr(0, qpos);
    
    if (script_name.empty() || script_name.find('.') == std::string::npos || script_name.substr(script_name.find_last_of('.')) != ".py")
        throw CGIException("Invalid script extension. Only .py scripts are supported.");

    std::string full_path = cgi_root;
    if (full_path[full_path.length() - 1] != '/')
        full_path += "/";
    full_path += script_name;
    Logger::debug("Full script path: " + full_path);
    if (access(full_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + full_path);
    return full_path;
}

void CGI::createAv(const std::string& cgi_path, const std::string& upload_to) 
{
    if (upload_to.empty()) 
    {
        Logger::warn("Upload directory is empty, defaulting to './uploads'");
        _upload_to = "./uploads";
    }
    if (!opendir(_upload_to.c_str())) 
    {
        Logger::debug("Creating upload directory: " + _upload_to);
        mkdir(_upload_to.c_str(), 0755);
    }
    _av = new char*[4];
    _av[0] = strdup("/usr/bin/python3");
    _av[1] = strdup(cgi_path.c_str());
    _av[2] = strdup(_upload_to.c_str());
    _av[3] = NULL;
    Logger::debug("CGI arguments: [" + std::string(_av[0]) + ", " + _av[1] + ", " + _av[2] + "]");
}

void CGI::createEnv(const HttpRequest& request) 
{
    std::map<std::string, std::string> headers = request.getHttpHeaders();
    size_t env_size = _env_map.size() + headers.size() + 1;
    _env = new char*[env_size];
    size_t i = 0;
    std::map<std::string, std::string>::const_iterator env_it;
    for (env_it = _env_map.begin(); env_it != _env_map.end(); ++env_it) 
    {
        std::string env_entry = env_it->first + "=" + env_it->second;
        _env[i++] = strdup(env_entry.c_str());
    }
    std::map<std::string, std::string>::const_iterator header_it;
    for (header_it = headers.begin(); header_it != headers.end(); ++header_it) 
    {
        std::string env_entry = "HTTP_" + header_it->first + "=" + header_it->second;
        _env[i++] = strdup(env_entry.c_str());
    }
    _env[i] = NULL;
}

void CGI::createAvAndEnv(const HttpRequest& request) 
{
    if (access(_cgi_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + _cgi_path);
    createAv(_cgi_path, _upload_to);
    createEnv(request);
}

// TO REMOVE?
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

void CGI::populateEnvVariables(const HttpRequest& request) 
{
    std::string uri = request.getHttpRequestLine()["request-uri"];
    size_t qpos = uri.find('?');
    _env_map["QUERY_STRING"] = qpos != std::string::npos ? uri.substr(qpos + 1) : "";
    _env_map["PATH_INFO"] = qpos != std::string::npos ? uri.substr(0, qpos) : uri;
    _env_map["REQUEST_METHOD"] = _request_method;
    _env_map["REQUEST_URI"] = uri;
    _env_map["SERVER_PROTOCOL"] = request.getHttpRequestLine()["protocol"];
    _env_map["SERVER_NAME"] = _serverInfo.getIP();
    _env_map["SERVER_PORT"] = _serverInfo.getPort();
    _env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
    if (_request_method == "POST") 
    {
        std::ostringstream oss;
        oss << _processed_body.size();
        _env_map["CONTENT_LENGTH"] = oss.str();
        std::map<std::string, std::string> headers = request.getHttpHeaders();
        std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
        if (it != headers.end()) _env_map["CONTENT_TYPE"] = it->second;
        it = headers.find("transfer-encoding");
        if (it != headers.end() && it->second == "chunked") 
        {
            std::string unchunked;
            if (unchunkRequest(_processed_body, unchunked)) 
            {
                _processed_body = unchunked;
                std::ostringstream oss2;
                oss2 << _processed_body.size();
                _env_map["CONTENT_LENGTH"] = oss2.str();
            }
        }
    } 
    else 
    {
        _env_map["CONTENT_LENGTH"] = "0";
    }
}

void CGI::startExecution() 
{
    if (pipe(_in_pipe) == -1 || pipe(_out_pipe) == -1 || pipe(_err_pipe) == -1) 
    {
        closePipes();
        throw CGIException("Failed to create pipes: " + std::string(strerror(errno)));
    }
    _pid = fork();
    if (_pid == -1) 
    {
        closePipes();
        throw CGIException("Failed to fork: " + std::string(strerror(errno)));
    }
    if (_pid == 0) 
    {
        close(_in_pipe[1]); close(_out_pipe[0]); close(_err_pipe[0]);
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 ||
            dup2(_out_pipe[1], STDOUT_FILENO) == -1 ||
            dup2(_err_pipe[1], STDERR_FILENO) == -1) 
        {
            exit(1);
        }
        close(_in_pipe[0]); close(_out_pipe[1]); close(_err_pipe[1]);
        execve(_av[0], _av, _env);
        exit(1);
    } 
    else // Parent process
    {
        close(_in_pipe[0]); // Close read end of input pipe
        close(_out_pipe[1]); // Close write end of output pipe
        close(_err_pipe[1]); // Close write end of error pipe
   
        if (_processed_body.empty())
        {
            close(_in_pipe[1]); // Close write end if no data to send
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("No body to write, input done! Yay!");
        }
    }
}

void CGI::appendOutput(const char* data, size_t len) 
{
    _output.append(data, len);
}

void CGI::writeInput() 
{
    if (_input_done || _in_pipe[1] == -1) 
    {
        Logger::debug("CGI writeInput skipped: already done or pipe closed. We good!");
        return;
    }
    size_t remaining = _processed_body.size() - _bytes_written;
    if (remaining <= 0) 
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        Logger::debug("CGI input completed. Nice.");
        return;
    }
    const size_t CHUNK_SIZE = 1024;
    size_t to_write = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
    ssize_t written = write(_in_pipe[1], _processed_body.c_str() + _bytes_written, to_write);
    if (written > 0) 
    {
        _bytes_written += written;
        Logger::debug("Wrote " + Utils::toString(written) + " bytes to CGI script.");
        
        if (_bytes_written >= _processed_body.size()) 
        {
            close(_in_pipe[1]);
            _in_pipe[1] = -1;
            _input_done = true;
            Logger::debug("CGI input fully written and pipe closed. Let's go.");
        }
    } 
    else 
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
        _input_done = true;
        Logger::debug("CGI write failed or pipe closed. Yikes!");
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
    // closePipe(_in_pipe[1]);
    if (_in_pipe[1] != -1)  // Only close if still open
        closePipe(_in_pipe[1]);

    closePipe(_out_pipe[0]);
    closePipe(_out_pipe[1]);
    closePipe(_err_pipe[0]);
    closePipe(_err_pipe[1]);
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
        Logger::warn("CGI process killed due to timeout. What a sad death :(");
    }
}