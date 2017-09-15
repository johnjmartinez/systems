#include "yash.h"

struct termios yash_modes;
job * head_job = NULL;

int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];
    
    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;
    
    yash_init();

    while(1) {
    
        printf("# ");
        fflush(stdout);
        skip = false;
        
        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            return(0);
        }   
        
        count = 0;
        skip = tokenizer(line, _tokens, &count);
        if (skip || (_tokens[0]==NULL)) continue;
        
        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser(_tokens, &pipe_pos, &fwd_pos, &bck_pos);
        if (skip) continue;
        
        if(!valid(pipe_pos, fwd_pos, bck_pos)) continue;
        
        // DEBUG printf ("p:%i f:%i b:%i cnt:%i\n", pipe_pos, fwd_pos, bck_pos, count);
        skip = executor(_tokens, pipe_pos, fwd_pos, bck_pos, count);
        if (skip) continue;
        
    }
    printf("\n");
    return(1);
}

void yash_init() {
    // Ignore interactive and job-control signals.
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    signal (SIGCHLD, SIG_IGN);

    yash_pgid = getpid ();                  // set yash group id
    if (setpgid (yash_pgid, yash_pgid) < 0) {
        perror ("ERROR: yahs init - setpgid");
        exit (1);
    }

    tcsetpgrp (STDIN_FILENO, yash_pgid);    // grab terminal control
    tcgetattr (STDIN_FILENO, &yash_modes);  // save default attrs
}
