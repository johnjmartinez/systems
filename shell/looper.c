#include "yash.h"

static void sig_int(int signo) {
    kill(-cid1, SIGINT);
}

static void sig_tstp(int signo) {
    kill(-cid1, SIGTSTP);
    fflush(stdout);
    tcsetpgrp (STDIN_FILENO, yash_pgid);
    tcgetattr (STDIN_FILENO, &yash_modes);  // Restore shell's terminal modes.
    return;
}

int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];

    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;

    job * j_curr =  NULL;
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
        if ( skip || (_tokens[0]==NULL) ) continue;

        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser(_tokens, &pipe_pos, &fwd_pos, &bck_pos);
        if (skip) continue;

        if(!valid(pipe_pos, fwd_pos, bck_pos)) continue;

        // new_job (j_curr, line);  // insert new job in list
        // DEBUG printf ("p:%i f:%i b:%i cnt:%i\n", pipe_pos, fwd_pos, bck_pos, count);
        executor(_tokens, pipe_pos, fwd_pos, bck_pos, count, j_curr);

    }
    printf("\n");
    return(1);
}

void yash_init() {
    
    head_job = NULL;
    
    signal (SIGQUIT, SIG_IGN);  // Ignore interactive and job-control signals.     
    signal (SIGTTIN, SIG_IGN);  // forked children signals will be set to default
    signal (SIGTTOU, SIG_IGN);  // later (launch_proc)
    signal (SIGCHLD, SIG_IGN);

    if (signal(SIGINT, sig_int)   == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, sig_tstp) == SIG_ERR) printf("signal(SIGTSTP) error");    
        
    yash_pgid = getpid ();                  // set yash group id
    if (setpgid (yash_pgid, yash_pgid) < 0) {
        perror ("ERROR: yash init - setpgid");
        exit (1);
    }

    tcsetpgrp (STDIN_FILENO, yash_pgid);    // grab terminal control
    tcgetattr (STDIN_FILENO, &yash_modes);  // save default attrs
}

void new_job (job * nj, char * line) {

        nj = malloc ( sizeof (job) );

        job * curr = head_job;
        while (curr->next != NULL)  curr = curr->next;
        
        curr = nj;
        nj->next = NULL;
        nj->cmd = line ;
        nj->head_proc = NULL;
        nj->pgid = 0;
        nj->notified = 0;
        nj->modes = yash_modes;
}
