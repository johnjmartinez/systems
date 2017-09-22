/**
 * @file TCPServer-ex1.c
 * @brief  Creates a tcp socket in the inet 
 *         domain, binds it to port given as argv[1], 
 *         listens and accepts a connection from 
 *         TCPClient-ex1, and receives any message
 *         arrived to the socket and prints it out
 * Run as:
 *    TCPServer-ex1 <hostname> <port>
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

int main(int argc, char** argv)
{
   int sd, psd;
   struct   sockaddr_in name;
   char   buf[1024];
   int    cc;

   sd = socket (AF_INET,SOCK_STREAM,0);
   name.sin_family = AF_INET;
   name.sin_addr.s_addr = htonl(INADDR_ANY);
   name.sin_port = htons(atoi(argv[1]));
   bind( sd, (struct  sockaddr *) &name, sizeof(name) );
   listen(sd,1);
   psd = accept(sd, 0, 0);
   for(;;) {
   	cc=recv(psd,buf,sizeof(buf), 0) ;
        if (cc == 0) exit (0);
   	buf[cc] = '\0';
   	printf("message received: %s\n", buf);
   }
}
