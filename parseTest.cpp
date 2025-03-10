#include "ClientHandler.hpp"
#include "InfoServer.hpp"

#include <iostream>

int main() {

	InfoServer info;
    std::string httpRequest =
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Length: 335\r\n"
        "Connection: keep-alive\r\n\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"example.txt\"\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "This is the content of the file.\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    // Output the constructed HTTP request
	ClientHandler *client = new ClientHandler(1, info);

	client->processClient();
	
	std::cout << *client << std::endl;
	delete client;
	
    return 0;
}