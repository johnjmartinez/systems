#ifndef CLIENT_H
#define CLIENT_H

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h> // sockaddr_in 
#include <netdb.h>      // struct hostent

#define LINE_MAX 204    // CMD/CTL +\ + <cmd>
#define	DELIM " \t\a\r" // took out \n so that client would recognize empty line as invalid
#define PORT_NUM 34567

int sckt;

bool tokenizer (char * line, char * _tokens[]);
int parser (char * _tokens[]);

#endif /* CLIENT_H */

