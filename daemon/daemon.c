#include "daemon.h"

int sock_fd, new_sock_fd, pid, log_fd;
struct sockaddr_in server_addr, client_addr;
char buff[512];


int main (int argc, char * argv[]) {
    
    d_init();     // daemon init
    s_init(argv); // socket init: socket(), bind(), listen()

    socklen_t client_len = sizeof(client_addr);
    
    while(1) {
        new_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_len);
        sprintf (buff, "%6d", new_sock_fd);
            
        write (log_fd, "\tNEW CONNECTION", 15);
        write (log_fd, buff, strlen(buff));
        write (log_fd, "\n", 1);

        // TODO -- create new thread with new_sock_fd
        // TODO -- log to LOGFILE?
           
         if ( (pid = fork ()) < 0)
             error_and_exit("ERROR on fork");
             
         if (pid == 0) {        // CHILD
             close(sock_fd);
             do_stuff(new_sock_fd);
             exit(0);
         }
         else 
            close(new_sock_fd);   // PARENT      
    }
    
    close(sock_fd);
    write (log_fd, "CLOSING SERVER\n", 17);
    close(log_fd);

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
        }
    }
    fprintf(stdout, "\n");
}

void s_init (char * argv[]) {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero ((void *)&server_addr, sizeof(server_addr)); // init to zeroes

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    server_addr.sin_port =  htons(PORT_NUM);

    bind ( sock_fd, (struct  sockaddr *) &server_addr, sizeof(server_addr) );
    listen ( sock_fd, MAX_CONNECTIONS ); 
    
    write (log_fd, "STARTING SERVER\n", 17);
}

void d_init() {
    
    pid_t pid;
    static FILE * log ; 
    int fd, pid_fd;

    if ( ( pid = fork() ) < 0 ) 
        error_and_exit("ERROR: d_init - cannot fork");
    else if (pid > 0) 
        exit(0);                    // PARENT EXITS - server to background

    for (fd = getdtablesize()-1; fd>0; fd--) 
        close(fd);                  // close any open fds

    if ( (fd = open("/dev/null", O_RDWR)) < 0) 
        error_and_exit("ERROR: d_init - can't open /dev/null");
    dup2(fd, STDIN_FILENO);         // redirect STDIN to /dev/null
    dup2(fd, STDOUT_FILENO);        // redirect STDOUT to /dev/null
    close (fd);

    log = fopen(LOG_FILE, "aw");    // open: write + append     
    log_fd = fileno(log);               // get log file descrpt
    dup2(log_fd, STDERR_FILENO);        // redirect STDERR to log
    //close (fd);

    /* TODO -- SIGCHLD & SIGPIPE
    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_and_exit("Signal SIGCHLD");
    if ( signal(SIGPIPE, sig_pipe) < 0 ) 
        error_and_exit("Signal SIGPIPE");
     */
    
    chdir("/tmp");    
    //umask(0);                     // set umask 
    setsid();                       // becoming session leader 
    pid = getpid();                 // put self in new process group 
    setpgrp();        

    // Make sure only one server is running
    if ( ( pid_fd = open("yashd.pid", O_RDWR | O_CREAT, 0666) ) < 0 ) 
            exit(1);
    if ( lockf(pid_fd, F_TLOCK, 0) != 0 ) 
            exit(0);

    // Save server's pid w/o closing file (so lock remains)
    sprintf (buff, "%6d", pid);
    write (pid_fd, buff, strlen(buff));
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
            return;
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

void do_stuff (int sock) {
   int n;
   char buffer[512];
      
   bzero (buffer, 512);
   n = read (sock, buffer, 511);
   if (n < 0) 
       error_and_exit("ERROR reading from socket");
   
   fprintf (stderr, "\t message: %s\n", buffer);
   n = write(sock, "From server:msg received", 24);
   if (n < 0) 
       error_and_exit("ERROR writing to socket");
}

void error_and_exit(const char *msg) {
    perror(msg);
    exit(1);
}

