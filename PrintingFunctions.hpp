#ifndef PRINTING_FUNCTIONS_H
#define PRINTING_FUNCTIONS_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>

void printIpvVersion(struct sockaddr_storage address);

void printError(int error);

void infoRecvLoop(int number, int bytes, char *buffer, std::string full_str, int size, int accumilating_size);


#endif