#ifndef PRINTING_FUNCTIONS_H
#define PRINTING_FUNCTIONS_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>
#include <map>

void printIpvVersion(struct sockaddr_storage address);

void printError(int error);

void infoRecvLoop(int number, int bytes, char *buffer, std::string full_str, int size, int accumilating_size);

void printFcntlFlag(int fd);

int printRecvFlag(int result);

void printReqHttpMap(std::map<std::string, std::string> myMap);

// template<typename T, typename U, typename V>
// void printMap(std::map<T, U> outerrMap, std::map<V, V> innerMap)
// {
// 	typename std::map<T, U>::iterator outerIt;
// 	typename std::map<V, V>::iterator innverIt;
// 	for (outerIt = outerrMap.begin(); outerIt != outerrMap.end(); outerIt++)
// 	{
// 		std::cout << "Index: " << outerIt->first << std::endl;

// 		for (innverIt = innerMap.begin(); innverIt != innerMap.end(); innverIt++)
// 		{
// 			std::cout << innverIt->first << std::endl;
// 			std::cout << innverIt->second << std::endl;
// 		}
// 	}
// }


#endif