#include "client.h"
/*CLIENT = yash <SERVER_IP_ADDR> (port is implicit = 3826)
    protocol:
        CMD <cmd_string> \n
        CTL <[c|z|d]> \n
    end: 
        ctrl+d (EOF) or 'quit'
    signal handling:
        ctrl+c will send "CTL c\n" (stop cmd on server)
        ctrl+x will send "CTL z\n" (suspend cmd on server)
*/

static void catch_C(int signo) { // ctrl+c
    send (sckt, "CTL c \n", 7, 0);
}

static void catch_Z(int signo) { // ctrl+z
    send (sckt, "CTL z \n", 7, 0);
}

int main (int argc, char* argv[]) {
    
    if (signal(SIGINT, catch_C) == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_Z) == SIG_ERR) printf("signal(SIGTSTP) error");
    
    char line[LINE_MAX];
    char * _tokens[LINE_MAX / 3];
    char * tmp;

    int skip;
    struct sockaddr_in server;
    struct hostent * target;
       
    sckt = socket (AF_INET, SOCK_STREAM, 0);
    target = gethostbyname(argv[1]);
    server.sin_port = htons (PORT_NUM);
    server.sin_family = AF_INET;
    bcopy ( target->h_addr, &(server.sin_addr.s_addr), target->h_length );
    connect ( sckt, (struct  sockaddr *) &server, sizeof(server) );

    while (1) {

        // TODO -- get command prompt from server
        // TODO -- Handle SIGPIPE from server going down?
       printf("# ");
        fflush(stdout);
        skip = false;

        if (fgets(line, LINE_MAX, stdin) == NULL) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            return (0);  // TODO -- close connection?
        }

        tmp = strdup (line);
        skip = tokenizer (tmp, _tokens);
        if (skip || (_tokens[0] == NULL)) {
            free(tmp);
            continue;
        }

        skip = parser (_tokens);
        if (skip) {
            if (skip > 1) { // QUIT
                printf("\n");
                return (0); // TODO -- close connection?
            }
            free(tmp);
            continue;
        }

        send (sckt, line, strlen(line), 0 );
        free(tmp);
    }

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
    if (strncmp(_tokens[0], "CMD", 3) == 0) {
        return 0;
    }
    else if (strncmp(_tokens[0], "CTL", 3) == 0) {
        return 0;
    }
    else if (strncmp(_tokens[0], "quit", 4) == 0) {
        return 2; // TODO -- check only token: _tokens[1] == NULL ? 
    }
    printf(" -ERROR: INVALID COMMAND\n");
    return 1; 
}
