#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h> // sockaddr_in 
#include <netdb.h>      // struct hostent

#define LINE_MAX 200    // CMD/CTL + \s + <cmd>
#define	DELIM " \t\a\r" // took out \n so that client would recognize empty line as invalid
#define PORT_NUM 3826

int sckt_fd;
bool connection_error;
pthread_t recv_t;
char OUT_BUFFER[204];

bool tokenizer (char * line, char * _tokens[]);
int parser (char * _tokens[]);
void * listen_n_display (void * arg) ;
void error_n_exit (const char *msg);
