#include "ClientRequest.hpp"

#include <iostream>

int main() {
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
	ClientRequest client;
	// std::map<int, struct section> sections;
	// std::map<std::string, std::string> headers;
	client.parseRequestHttp(httpRequest.c_str(), httpRequest.length());
	// headers = client.getHttpHeaders();
	// std::cout << "headers\n";
	// for (it = headers.begin(); it != headers.end(); it++)
	// {
	// 		std::cout << it->first << " : ";
	// 		std::cout << it->second << std::endl;
	// }
	std::cout << "sections\n";
	std::vector<struct section> sections;
	sections = client.getSections();
	std::vector<struct section>::iterator it;
	// std::map<std::string, std::string>::iterator innerIt;
	// //printf("here\n");
	int i = 0;
	for (it = sections.begin(); it != sections.end(); it++)
	{
		//printf("in loop\n");
		for (std::map<std::string, std::string>::iterator mapIt = it->myMap.begin(); mapIt != it->myMap.end(); ++mapIt) {
            std::cout << mapIt->first << ": " << mapIt->second << std::endl;
        }
		std::cout << it->body << std::endl;
		std::cout << it->indexBinary << std::endl;
	}
    return 0;
}