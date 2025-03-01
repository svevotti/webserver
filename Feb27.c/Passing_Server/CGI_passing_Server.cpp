#include "CGI.hpp"
#include "ClientRequest.hpp"
#include "Server.hpp"

const std::string CGI::SCRIPT_BASE_DIR = "/www/cgi-bin/";
const int CGI::TIMEOUT_SECONDS = 5;

CGI::CGI(const ClientRequest& request, const std::string& upload_to, const Server& server)
    : _request_method(request.getRequestLine()["Method"]), _upload_to(upload_to),
      _processed_body(""), _av(NULL), _env(NULL), _output(""),
      _in_pipe{-1, -1}, _out_pipe{-1, -1}, _pid(-1), _bytes_written(0),
      _input_done(false), _output_done(false), _start_time(time(NULL))
{
    try 
    {
        populateEnvVariables(request, server);
        _cgi_path = getScriptFileName(request, server);
        createAvAndEnv(request, server);
        processRequestBody(request);
        runScript();
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
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
        _pid = -1;
    }
}

std::string CGI::getScriptFileName(const ClientRequest& request, const Server& server) const
{
    std::string uri = request.getRequestLine()["Request-URI"];
    size_t pos = uri.find_last_of('/');
    std::string script_name = (pos != std::string::npos) ? uri.substr(pos + 1) : uri;
    size_t qpos = script_name.find('?');
    if (qpos != std::string::npos)
        script_name = script_name.substr(0, qpos);

    if (script_name.empty() || script_name.substr(script_name.find_last_of('.')) != ".py")
        throw CGIException("Invalid script extension. Only .py scripts are supported.");

    std::string cgi_root = server.getRoot() + "/cgi-bin/";
    if (script_name[0] != '/')
        script_name = cgi_root + script_name;
    else if (script_name.find(cgi_root) != 0)
        throw CGIException("Script path must be under " + cgi_root);

    return script_name;
}

void CGI::createAvAndEnv(const ClientRequest& request, const Server& server)
{
    if (access(_cgi_path.c_str(), F_OK) != 0)
        throw CGIException("Script file does not exist: " + _cgi_path);

    _av = new char*[4];
    _av[0] = strdup("/usr/bin/python3");
    _av[1] = strdup(_cgi_path.c_str());
    _av[2] = strdup(_upload_to.c_str());
    _av[3] = NULL;

    std::map<std::string, std::string> requestLine = request.getRequestLine();
    std::map<std::string, std::string> headers = request.getHeaders();
    std::map<std::string, std::string> query = request.getQueryMap();
    size_t env_size = 6 + headers.size() + query.size();
    _env = new char*[env_size + 1];
    size_t i = 0;

    _env[i++] = strdup(("REQUEST_METHOD=" + _request_method).c_str());
    _env[i++] = strdup(("REQUEST_URI=" + requestLine["Request-URI"]).c_str());
    _env[i++] = strdup(("SERVER_PROTOCOL=" + requestLine["Protocol"]).c_str());
    _env[i++] = strdup(("SERVER_NAME=" + server.getIP()).c_str());
    _env[i++] = strdup(("SERVER_PORT=" + server.getPort()).c_str());
    _env[i++] = strdup("GATEWAY_INTERFACE=CGI/1.1");

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

bool CGI::unchunkRequest(const std::string& chunked, std::string& unchunked)
{
    std::istringstream iss(chunked);
    std::string line;
    long chunk_size;

    while (std::getline(iss, line)) 
    {
        if (line.empty())
            continue;

        for (std::string::const_iterator it = line.begin(); it != line.end(); ++it) 
        {
            if (!isxdigit(*it))
                throw CGIException("Invalid hexadecimal string in chunked request.");
        }

        char* end_ptr;
        errno = 0;
        chunk_size = std::strtol(line.c_str(), &end_ptr, 16);

        if ((errno == ERANGE && (chunk_size == LONG_MAX || chunk_size == LONG_MIN)) ||
            (end_ptr == line.c_str()) || (*end_ptr != '\0'))
            throw CGIException("Invalid chunk size in chunked request.");

        if (chunk_size == 0)
            break;

        std::vector<char> buffer(chunk_size);
        iss.read(buffer.data(), chunk_size);
        if (iss.gcount() != static_cast<std::streamsize>(chunk_size))
            throw CGIException("Failed to read chunk data.");

        unchunked.append(buffer.data(), chunk_size);
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
        if (headers.find("Transfer-Encoding") != headers.end() && headers["Transfer-Encoding"] == "chunked") {
            if (!unchunkRequest(request.getBodyText(), _processed_body))
                throw CGIException("Failed to unchunk POST request body.");
        } 
        else
            _processed_body = request.getBodyText();
    } 
    else
        throw CGIException("Unsupported REQUEST_METHOD: " + _request_method);
}

void CGI::populateEnvVariables(const ClientRequest& request, const Server& server)
{
    std::map<std::string, std::string> requestLine = request.getRequestLine();
    std::string uri = requestLine["Request-URI"];
    size_t qpos = uri.find('?');
    if (qpos != std::string::npos) 
    {
        _env_var["QUERY_STRING"] = uri.substr(qpos + 1);
        _env_var["PATH_INFO"] = uri.substr(0, qpos);
    } 
    else 
    {
        _env_var["QUERY_STRING"] = "";
        _env_var["PATH_INFO"] = uri;
    }
    _env_var["CONTENT_LENGTH"] = (_request_method == "POST") ? std::to_string(_processed_body.size()) : "0";
    _env_var["SERVER_PORT"] = server.getPort();
    _env_var["SERVER_NAME"] = server.getIP();
}

void CGI::runScript()
{
    if (pipe(_in_pipe) == -1)
        throw CGIException("Failed to create input pipe: " + std::string(strerror(errno)));
    
    if (pipe(_out_pipe) == -1) 
    {
        close(_in_pipe[0]);
        close(_in_pipe[1]);
        throw CGIException("Failed to create output pipe: " + std::string(strerror(errno)));
    }

    _pid = fork();
    if (_pid == -1) 
    {
        close(_in_pipe[0]);
        close(_in_pipe[1]);
        close(_out_pipe[0]);
        close(_out_pipe[1]);
        throw CGIException("Failed to fork: " + std::string(strerror(errno)));
    }

    if (_pid == 0) 
    {
        close(_in_pipe[1]);
        close(_out_pipe[0]);
        if (dup2(_in_pipe[0], STDIN_FILENO) == -1 || dup2(_out_pipe[1], STDOUT_FILENO) == -1) 
        {
            std::cerr << "dup2 failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        close(_in_pipe[0]);
        close(_out_pipe[1]);
        if (execve(_av[0], _av, _env) == -1) 
        {
            std::cerr << "execve failed: " << strerror(errno) << std::endl;
            exit(1);
        }
    } 
    else 
    {
        close(_in_pipe[0]);
        close(_out_pipe[1]);
        // No initPollFds(). SocketServer handles poll_sets directly
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
        // No poll_sets update here—Server handles it
    }
}

bool CGI::isDone() const
{
    if (_pid <= 0)
        return true;
    int status;
    pid_t result = waitpid(_pid, &status, WNOHANG);
    bool process_exited = (result == _pid);
    return _input_done && _output_done && process_exited;
}

bool CGI::isTimedOut() const
{
    return (time(NULL) - _start_time >= TIMEOUT_SECONDS);
}

void CGI::closePipes()
{
    if (_in_pipe[1] != -1) 
    {
        close(_in_pipe[1]);
        _in_pipe[1] = -1;
    }
    if (_out_pipe[0] != -1) 
    {
        close(_out_pipe[0]);
        _out_pipe[0] = -1;
    }
}