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
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"submit\"\r\n\r\n"
        "Upload\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    // Output the constructed HTTP request
	ClientRequest client;
	std::map<int, struct header> sections;
	std::map<std::string, std::string> headers;
	client.parseRequestHttp(httpRequest.c_str(), httpRequest.length());
	headers = client.getHeaders();
	std::map<std::string, std::string>::iterator it;
	std::cout << "headers\n";
	for (it = headers.begin(); it != headers.end(); it++)
	{
			std::cout << it->first << " : ";
			std::cout << it->second << std::endl;
	}
	std::cout << "sections\n";
	sections = client.getBodySections();
	std::map<int, struct header>::iterator outerIt;
	std::map<std::string, std::string>::iterator innerIt;
	//printf("here\n");
	for (outerIt = sections.begin(); outerIt != sections.end(); outerIt++)
	{
		//printf("in loop\n");
		struct header section = outerIt->second;
		std::cout << "section size: " << section.myMap.size() << std::endl;
		for (innerIt = section.myMap.begin(); innerIt != section.myMap.end(); innerIt++)
		{
			std::cout << innerIt->first << " : ";
			std::cout << innerIt->second << std::endl;
		}
	}
    return 0;
}