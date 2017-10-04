/* CLIENT
 @brief u-echo.c - Communicate with u-echod server through Unix socket associated with file pathname 
    [passed either as argv[1] or by default /tmp/u-echod] In loop user is prompted to enter line of text
    and data is sent to server; reply (echo) is printed out. loop terminates when we enter null line.
 (https://cis.temple.edu/~ingargio/cis307/readings/daemon.html)
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

int main(int argc, char * argv[]) {
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
        
        /* Read line from user and write it to socket */
        n = 0;
        printf("Enter line: ");
        for (;;) {
            c = getchar();
            write(sockfd, &c, 1);
            if (c == '\n') break;
            n++;
        }
        /* Read line from socket */
        for (;;) {
            read(sockfd, &c, 1);
            putchar(c);
            if (c == '\n') break;
        }
        if (n == 0) break; /* line entered by user was null */
    }
    
    exit(0);
}
