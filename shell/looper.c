#include "yash.h"

int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];
    char * tmp;

    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;

    yash_init();

    while(1) {

        job_notify();
        fflush(stdout);

        fprintf(stdout, "# ");
        fflush(stdout);
        skip = false;

        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            kill_jobs();
            return(0);
        }

        tmp =  strdup (line);
        count = 0;

        skip = tokenizer (tmp, _tokens, &count);
        if ( skip || (_tokens[0]==NULL) ) {
            free (tmp);
            continue;
        }

        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser (_tokens, &pipe_pos, &fwd_pos, &bck_pos);
        if (skip) {
            free (tmp);
            continue;
        }

        if (!valid (pipe_pos, fwd_pos, bck_pos)) {
            free (tmp);
            continue;
        }

        // /*DEBUG*/ printf ("p:%i f:%i b:%i cnt:%i\n", pipe_pos, fwd_pos, bck_pos, count);
        executor (_tokens, pipe_pos, fwd_pos, bck_pos, count, line);
        free (tmp);
    }
    printf("\n");
    return(1);
}

// ONLY APPLY TO FG (stopped or running) JOBS -- hence using cgid
static void catch_INT (int signo) { // ctrl+c

    job * i;
    if (cgid) {
        if ( (i = find_job (cgid)) == NULL )
            fprintf(stderr, "YASH catch_INT: cgid %d not found\n", cgid);
        else {
            i->done = 1;
            kill(cgid, SIGINT);
            fflush(stdout);
        }
    }
    fprintf(stdout, "\n");
}

// ONLY APPLY TO FG (running) JOBS  -- hence using cgid
static void catch_TSTP(int signo) {   // ctrl+z

    job * i;
    if (cgid) {
        if ( (i = find_job (cgid)) == NULL )
            fprintf(stderr, "YASH catch_TSTP: cgid %d not found\n", cgid);
        else {
            i->paused = 1;
            kill(cgid, SIGTSTP);
            fflush(stdout);
            tcsetpgrp (STDIN_FILENO, yash_pgid);
            //tcgetattr (STDIN_FILENO, &yash_modes);    // Restore shell's terminal modes.
        }
    }
    fprintf(stdout, "\n");
}

void yash_init() {

    head_job = NULL;

    if (signal(SIGINT, catch_INT) == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_TSTP) == SIG_ERR) printf("signal(SIGTSTP) error");

    yash_pgid = getpid ();                      // set yash group id
    if (setpgid (yash_pgid, yash_pgid) < 0) {
        perror ("ERROR: yash init - setpgid");
        exit (1);
    }

    tcsetpgrp (STDIN_FILENO, yash_pgid);        // grab terminal control
    //tcgetattr (STDIN_FILENO, &yash_modes);    // save default attrs
}
