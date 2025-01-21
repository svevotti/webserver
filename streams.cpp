#include <sstream>
#include <string>
#include <iostream>


int main(void)
{
    std::string str;
    
    str =	"HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 103\r\n" //need to exactly the message's len, or it doesn't work
            "\r\n"
            "<html>\r\n"
            "<header>\r\n"
            "<title>Not Found</title>\r\n"
            "</header>\r\n"
            "<body>\r\n"
            "<h1>Not Found!!</h1>\r\n"
            "</body>\r\n"
            "</html>\r\n";
    
    std::istringstream strStream(str);
    std::string line;

    // strStream >> str;

    while (std::getline(strStream, line))
    {
        std::cout << "line - " << line << std::endl;
    }
}