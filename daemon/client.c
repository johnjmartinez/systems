#include "client.h"
/*CLIENT = yash <SERVER_IP_ADDR> (port is implicit = 3826)
    protocol:
        CMD <cmd_string> \n
        CTL <[c|z|d]> \n
    end: 
        ctrl+d (EOF) or 'quit'
    signal handling:
        ctrl+c will send "CTL c\n" (stop cmd on server)
        ctrl+z will send "CTL z\n" (suspend cmd on server)
*/

static void catch_C(int signo) { // ctrl+c
    send (sckt_fd, "CTL c \n", 7, 0);
}

static void catch_Z(int signo) { // ctrl+z
    send (sckt_fd, "CTL z \n", 7, 0);
}

static void catch_PIPE(int signo) { // sig pipe 
    pthread_join(recv_t, NULL);   
    exit(1);
}

int main (int argc, char* argv[]) {
    
    //setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
       fprintf(stderr,"Usage: %s <hostname>\n", argv[0]);
       exit(0);
    }
    
    if (signal(SIGINT, catch_C) == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_Z) == SIG_ERR) printf("signal(SIGTSTP) error");
    if (signal(SIGPIPE, catch_PIPE) == SIG_ERR) printf("signal(SIGPIPE) error");

    char line[LINE_MAX];
    char * _tokens[LINE_MAX / 3];
    char * tmp;

    int skip, quit;
    struct sockaddr_in server;
    struct hostent * target;
       
    sckt_fd = socket (AF_INET, SOCK_STREAM, 0);
    target = gethostbyname(argv[1]);
    server.sin_port = htons (PORT_NUM);
    server.sin_family = AF_INET;
    bcopy ( target->h_addr, &server.sin_addr.s_addr, target->h_length );
    if ( connect ( sckt_fd, (struct  sockaddr *) &server, sizeof(server) ) < 0)
        error_n_exit ("ERROR connecting");
    
    pthread_create (&recv_t, NULL, listen_n_display, NULL);
    connection_error = false;
    do {
        
        skip = false;
        if (fgets(line, LINE_MAX, stdin) == NULL)  // catch ctrl+d (EOF) on empty line
            break;
        
        tmp = strdup (line);
        skip = tokenizer (tmp, _tokens);
        if (skip || (_tokens[0] == NULL)) {
            free(tmp);
            continue;
        }

        quit = parser (_tokens);
        if (quit)
            break;
            
        snprintf (OUT_BUFFER, 204, "CMD %s", line);
        write (sckt_fd, OUT_BUFFER, strlen(OUT_BUFFER));
        free(tmp);

    } while (!connection_error);
    
    close (sckt_fd);    
    printf("\n");
    return (EXIT_SUCCESS);
}

bool tokenizer (char * line, char * _tokens[]) {

    if (line[strlen(line) - 1] != '\n') {   // check if line too long
        printf("ERROR: command too long -- 200 chars max\n");
        while (getchar() != '\n');          // flush stdin
        return true;
    }

    int i = 0;
    char * pointer = strtok(line, DELIM);   // strtok destroys string
    do {
        _tokens[i++] = pointer;
    } while ((pointer = strtok(NULL, DELIM)) != NULL);
    
    return false;
}

int parser (char * _tokens[]) {

    if (strncmp(_tokens[0], "quit", 4) == 0) 
        return 1; // TODO -- check only token: _tokens[1] == NULL ? 
    
    return 0; 
}

void * listen_n_display (void * arg) { // THREAD
    
    int num;
    char buf[4096];
    
    for (;;) {
        bzero(buf, 4096);
        if ( (num = read (sckt_fd, buf, sizeof(buf))) < 0 ) {
            perror("ERROR: receiving stream msg");
            break;
        }
        else if (num) {
            fflush(stdout);
            buf[num]='\0';
            printf("%s", buf);
            fflush(stdout);
        }
        else {    
            printf("ERROR: Disconnected");
            close (sckt_fd);
            break;
        }
    }
    connection_error = true;
    pthread_exit(NULL);
}

void error_n_exit (const char *msg) {
    perror (msg);
    _exit(0);
}