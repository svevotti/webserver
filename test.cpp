#include "ClientRequest.hpp"

int main(void)
{
    ClientRequest client;

    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::cout << boundary.length() << std::endl;
    std::string request = 
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: multipart/form-data; boundary=------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Length: 1234\r\n"
        "Connection: close\r\n\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"file.txt\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";
    client.parseRequestHttp(request.c_str());
}