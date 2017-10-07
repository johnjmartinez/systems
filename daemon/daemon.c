#include "daemon.h"

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

int main(int argc, char * argv[]) {

    // TODO -- make process a daemon
    d_init(argv); // socket(), bind(), listen()
    // TODO -- set log output

    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        // TODO -- create new thread with newsockfd

    }
    
    close(sockfd);
    printf("\n");
    return(1);
}

// ONLY APPLY TO FG (stopped or running) JOBS -- hence using cgid
static void catch_C (int signo) { // ctrl+c

    job * i;
    if (cgid) {
        if ( (i = find_job (cgid)) == NULL )
            fprintf(stderr, "YASH catch_C: cgid %d not found\n", cgid);
        else {
            i->done = 1;
            kill(cgid, SIGINT);
            fflush(stdout);
        }
    }
    fprintf(stdout, "\n");
}

// ONLY APPLY TO FG (running) JOBS  -- hence using cgid
static void catch_Z(int signo) {   // ctrl+z

    job * i;
    if (cgid) {
        if ( (i = find_job (cgid)) == NULL )
            fprintf(stderr, "YASH catch_Z: cgid %d not found\n", cgid);
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

void d_init (char * argv[]) {

    head_job = NULL;

    if (signal(SIGINT, catch_C)  == SIG_ERR) printf("signal(SIGINT) error");
    if (signal(SIGTSTP, catch_Z) == SIG_ERR) printf("signal(SIGTSTP) error");

    yash_pgid = getpid ();                      // set yash group id
    if (setpgid (yash_pgid, yash_pgid) < 0) {
        perror ("ERROR: yash init - setpgid");
        exit (1);
    }

    tcsetpgrp (STDIN_FILENO, yash_pgid);        // grab terminal control
    //tcgetattr (STDIN_FILENO, &yash_modes);    // save default attrs
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    serv_addr.sin_port =  htons(atoi(argv[1]));

    bind ( sockfd, (struct  sockaddr *) &serv_addr, sizeof(serv_addr) );
    listen ( sockfd, 7 ); // MAX 7 connections in queue
}

void shell_job () { // XXX -- THREAD JOB : not sure what should be mutex'd here 
    
    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];
    char * tmp;

    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;
    
    for(;;) {
    
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

        executor (_tokens, pipe_pos, fwd_pos, bck_pos, count, line);
        free (tmp);
    }
}
