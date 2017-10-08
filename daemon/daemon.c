#include "daemon.h"

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

int main(int argc, char * argv[]) {

    d_init();
    s_init(argv); // socket(), bind(), listen()

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

void s_init (char * argv[]) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero((void *)&servaddr, sizeof(servaddr)); // init to zeroes
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    serv_addr.sin_port =  htons(atoi(argv[1]));

    bind ( sockfd, (struct  sockaddr *) &serv_addr, sizeof(serv_addr) );
    listen ( sockfd, 7 ); // MAX 7 connections in queue
}

void d_init() {
    
    pid_t pid;
    char buff[64];
    static FILE * log ; 
    int fd, pid_fd;

    if ( ( pid = fork() ) < 0 ) 
        error_and_exit("ERROR: d_init - cannot fork");
    else if (pid > 0) 
        exit(0);                    // PARENT EXITS - server to background

    for (fd = getdtablesize()-1; fd>0; fd--) 
        close(fd);                  // close any open fds

    if ( (fd = open("/dev/null", O_RDWR)) < 0) 
        error_and_exit("ERROR: d_init - /dev/null open");
    dup2(fd, STDIN_FILENO           // redirect STDIN to /dev/null
    dup2(fd, STDOUT_FILENO);        // redirect STDOUT to /dev/null
    close (fd);

    log = fopen(LOG_FILE, "aw");    // open: write + append     
    fd = fileno(log);               // get log file descrpt
    dup2(fd, STDERR_FILENO);        // redirect STDERR to log
    close (fd);

    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_and_exit("Signal SIGCHLD");
    if ( signal(SIGPIPE, sig_pipe) < 0 ) 
        error_and_exit("Signal SIGPIPE");

    chdir("/tmp");    
    umask(0);                       // set umask 
    setsid();                       // becoming session leader 
    pid = getpid();                 // put self in new process group 
    setpgrp();        

    // Make sure only one server is running
    if ( ( pid_fd = open("yashd.pid", O_RDWR | O_CREAT, 0666) ) < 0 ) 
            exit(1);
    if ( lockf(pid_fd, F_TLOCK, 0) != 0 ) 
            exit(0);

    // Save server's pid w/o closing file (so lock remains)
    sprintf(buff, "%6d", pid);
    write(pid_fd, buff, strlen(buff));
}

void shell_job () { // XXX -- THREAD JOB : not sure what should be mutex'd here 
    
    head_job = NULL;

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

void error_and_exit(const char *msg) {
    perror(msg);
    exit(1);
}

