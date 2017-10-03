#include "client.h"

/*
CLIENT = yash <SERVER_IP_ADDR> (port is implicit = 3826)
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

    // TODO -- send CTL c \n to server
    fflush(stdout);
    fprintf(stdout, "\n");
}

static void catch_Z(int signo) { // ctrl+z

    // TODO -- send CTL z \n to server
    fflush(stdout);
    fprintf(stdout, "\n");
}

int main(int argc, char* argv[]) {
    
    // TODO -- get arg1:SERVER_IP 
    
    char line[LINE_MAX];
    char * _tokens[LINE_MAX / 3];
    char * tmp;

    int count;
    bool skip;

    if (signal(SIGINT, catch_C) == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_Z) == SIG_ERR) printf("signal(SIGTSTP) error");
    
    // TODO -- start connection to server

    while (1) {

        // TODO -- get command prompt from server
        fflush(stdout);
        skip = false;

        if (fgets(line, LINE_MAX, stdin) == NULL) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            return (0);  // TODO -- close connection?
        }

        tmp = strdup(line);
        count = 0;

        skip = tokenizer(tmp, _tokens, &count);
        if (skip || (_tokens[0] == NULL)) {
            free(tmp);
            continue;
        }

        skip = parser(_tokens);
        if (skip) {
            if (skip > 1) {
                printf("\n");
                return (0); // TODO -- close connection?
            }
            free(tmp);
            continue;
        }

        // TODO -- send cmd to server once it's valid : send_cmd(line)
        free(tmp);
    }

    return (EXIT_SUCCESS);
}

bool tokenizer(char * line, char * _tokens[], int * count) {

    if (line[strlen(line) - 1] != '\n') { // check if line too long
        printf("ERROR: command too long -- 200 chars max\n");
        while (getchar() != '\n'); // flush stdin
        return true;
    }

    int i = 0;
    char * pointer = strtok(line, DELIM); // strtok destroys string
    do {
        _tokens[i++] = pointer;
    } while ((pointer = strtok(NULL, DELIM)) != NULL);

    _tokens[i] = NULL; // make last token/cmd_arg NULL
    * count = i;

    return false;
}

int parser(char * _tokens[]) {
    
    // XXX -- check first token is either CMD, CTL or quit
    if (strncmp(_tokens[0], "CMD", 3) == 0) {
        return 0;
    }
    else if (strncmp(_tokens[0], "CTL", 3) == 0) {
        return 0;
    }
    else if (strncmp(_tokens[0], "quit", 4) == 0) {
        return 2; // TODO -- check only token: _tokens[1] == NULL ? 
    }
    return 1; // TODO -- print ERROR msg?
}
