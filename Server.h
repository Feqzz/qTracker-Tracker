#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
class Server
{
public:
	Server(int _port);
	
	void central();
	void handle_client(int newsockfd);
	
private:
	int sockfd;
	int port;
	int pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	static const int buffersize = 4096;
	char buffer[buffersize];

	std::string array_to_string(char* arr, int size);
};

#endif
