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

int main (int argc, char* argv[]) {
    
    if (argc < 2) {
       fprintf(stderr,"Usage: %s hostname\n", argv[0]);
       exit(0);
    }
    
    if (signal(SIGINT, catch_C) == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_Z) == SIG_ERR) printf("signal(SIGTSTP) error");
    
    char line[LINE_MAX];
    char * _tokens[LINE_MAX / 3];
    char * tmp;

    int skip, num;
    struct sockaddr_in server;
    struct hostent * target;
       
    sckt_fd = socket (AF_INET, SOCK_STREAM, 0);
    target = gethostbyname(argv[1]);
    server.sin_port = htons (PORT_NUM);
    server.sin_family = AF_INET;
    bcopy ( target->h_addr, &server.sin_addr.s_addr, target->h_length );
    if ( connect ( sckt_fd, (struct  sockaddr *) &server, sizeof(server) ) < 0)
        error_n_exit ("ERROR connecting");
    // TODO -- Handle SIGPIPE from server going down?

    for(;;) {
        fflush(stdout);
        skip = false;

        // TODO -- figure out a way to check if receiving/writing constantly
        // TODO -- fix receives for rand len strs
        num = read (sckt_fd, buf, sizeof(buf));
        if (num < 0)
            error_n_exit ("ERROR: receiving stream msg");
            
        if (num) {
            buf[num]='\0';
            printf("%s", buf);
        }
        else {    
            printf("Disconnected? ...");
            break;
        } 

        if (fgets(line, LINE_MAX, stdin) == NULL)  // catch ctrl+d (EOF) on empty line
            break;
        
        tmp = strdup (line);
        skip = tokenizer (tmp, _tokens);
        if (skip || (_tokens[0] == NULL)) {
            free(tmp);
            continue;
        }

        skip = parser (_tokens);
        if (skip) {
            if (skip > 1)  // QUIT
                break;
            free(tmp);
            continue;
        }

        write (sckt_fd, line, strlen(line));
        free(tmp);
    }
        
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
    
    // ERROR CHECKING -- check first token is either CMD, CTL or quit
    if (strncmp(_tokens[0], "CMD", 3) == 0) 
        return 0;
    else if (strncmp(_tokens[0], "CTL", 3) == 0) 
        return 0;
    else if (strncmp(_tokens[0], "quit", 4) == 0) 
        return 2; // TODO -- check only token: _tokens[1] == NULL ? 
    
    printf(" -ERROR: INVALID COMMAND\n");
    return 1; 
}

void error_n_exit (const char *msg) {
    perror (msg);
    exit(0);
}