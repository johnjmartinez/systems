/**
 * @file TCPClient-ex1.c
 * @brief Ccreates a tcp socket in the inet 
 *         domain, connects to TCPServer-ex1.c running at <hostname> 
 *         and waiting at port ???, sends the message "HI" and exits
 * Run as:
 *    TCPClient-ex1 <hostname> <port>
 */
#include <stdio.h>
/* socket(), bind(), recv, send */
#include <sys/types.h>
#include <sys/socket.h> /* sockaddr_in */
#include <netinet/in.h> /* inet_addr() */
#include <arpa/inet.h>
#include <netdb.h> /* struct hostent */
#include <string.h> /* memset() */
#include <unistd.h> /* close() */
#include <stdlib.h> /* exit() */

int main(int argc, char **argv ) {
    int	sd;
    struct  sockaddr_in server;
    struct hostent *hp;
    
    sd = socket (AF_INET,SOCK_STREAM,0);
    
    server.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    server.sin_port = htons(atoi(argv[2]));
    connect(sd, (struct  sockaddr *) &server, sizeof(server));
    for (;;) {
	send(sd, "HI", 2, 0 );
	printf("sent HI\n");
	sleep(2);
    }
}

