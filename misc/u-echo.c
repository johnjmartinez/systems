/**
 * @file u-echo.c
 * @brief u-echo.c - Communicate with the u-echod server
 *       through a Unix socket associated with a file pathname 
 *      [passed either as argv[1] or by default /tmp/u-echod]
 *      In a loop the user is prompted to enter a line of text
 *      and this data is sent to the server; the reply (echo) is printed
 *      out. The loop terminates when we enter a null line.
 * This example is from Giorgio Ingargiola's Unix Programming Course
 * at Temple University 
 * (https://cis.temple.edu/~ingargio/cis307/readings/daemon.html)
 * I modified it a little bit and annotated it with explanations
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

char u_echo_path[256] = "/tmp/u-echod"; /* default */


int
main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un servaddr;
    
    if ( (sockfd = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        perror("Socket");
        exit (1);
    }
    
    bzero((void *)&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, u_echo_path);
    
    if ( connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
    perror("Connect");
    exit (1);
    }
    
    for ( ; ; ) {
        int n;
        char c;
        
        /* Read a line from user and write it to socket */
        n = 0;
        printf("Enter a line: ");
        for (;;) {
            c = getchar();
            write(sockfd, &c, 1);
            if (c == '\n') break;
            n++;
        }
        /* Read a line from the socket */
        for (;;) {
            read(sockfd, &c, 1);
            putchar(c);
            if (c == '\n') break;
        }
        if (n == 0) break; /* The line entered by user was null */
    }
    
    exit(0);
}
