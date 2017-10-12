#include "daemon.h"

int sckt_fd, new_sckt_fd, curr_pid, pid_fd, log_fd, fd;
struct sockaddr_in server_addr, client_addr;
char buff[512];
pthread_t p;

int main () {
    
    d_init();   // daemon init
    s_init();   // socket init: socket(), bind(), listen()

    socklen_t client_len = sizeof(client_addr);
    //for (;;) {
        new_sckt_fd = accept(sckt_fd, (struct sockaddr *) &client_addr, &client_len);
        
        log_time();
        fprintf (stderr, "NEW CONNECTION at %6d", new_sckt_fd);

        // TODO -- create new thread with new_sckt_fd
        // TODO -- create thread pool (array) of size MAX_CONNECTIONS
        // TODO -- create bool array available_threads (stack?)
        // TODO -- while accepting new connections, join thread 
        
        pthread_create(&p, NULL, do_stuff, (void *) new_sckt_fd) ;
        pthread_join(p, NULL);
    //}
    close(new_sckt_fd);
    close(sckt_fd);
    close(pid_fd);
    
    log_time();
    fprintf (stderr, "CLOSING SERVER");
    close(log_fd);

    printf("\n");
    return(1);
}

/*// ONLY APPLY TO FG (stopped or running) JOBS -- hence using cgid
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
}*/

/*// ONLY APPLY TO FG (running) JOBS  -- hence using cgid
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
}*/

void s_init () {
    
    socklen_t size;
    log_time ();
    fprintf (stderr, "STARTING SERVER ... ");

    sckt_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( sckt_fd < 0 )
 	    error_and_exit ("ERROR: s_init() -- starting socket\n");
    
    bzero ((void *)&server_addr, sizeof(server_addr)); // init to zeroes
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT_NUM);
    
    reusePort (sckt_fd);
    if ( bind(sckt_fd, (struct  sockaddr *) &server_addr, sizeof(server_addr)) )
  	    error_and_exit("ERROR: s_init() -- binding socket\n");
       
    size = sizeof(server_addr);
    if ( getsockname (sckt_fd, (struct sockaddr *) &server_addr, &size) ) 
	    error_and_exit("ERROR: s_init() -- getting socket name\n");
    
    fprintf(stderr, "at port: %d", ntohs(server_addr.sin_port));
    listen ( sckt_fd, MAX_CONNECTIONS ); 
}

void d_init() {
    
    pid_t pid;
    static FILE * log_file;
    //static FILE * pid_file ; 

    if ( (pid = fork()) < 0 ) 
        error_and_exit("ERROR: d_init - cannot fork\n");
    else if (pid) 
        exit(0);                        // PARENT EXITS - server to background

    for (fd = getdtablesize()-1; fd>0; fd--) 
        close(fd);                      // close any open fds

    if ( (fd = open("/dev/null", O_RDWR)) < 0) 
        error_and_exit("ERROR: d_init - can't open /dev/null\n");
    dup2 (fd, STDIN_FILENO);            // redirect STDIN to /dev/null
    dup2 (fd, STDOUT_FILENO);           // redirect STDOUT to /dev/null
    close (fd);

    log_file = fopen (LOG_FILE, "aw");  // open: write + append     
    log_fd = fileno (log_file);         // get log file descrpt
    dup2 (log_fd, STDERR_FILENO);       // redirect STDERR to log

    /* TODO -- SIGCHLD & SIGPIPE
    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_and_exit("Signal SIGCHLD");
    if ( signal(SIGPIPE, sig_pipe) < 0 ) 
        error_and_exit("Signal SIGPIPE");
     */
    
    pid = setsid();                     // put self in new process group 
    setpgrp();        

    // Save server's pid 
    if ( (pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0666)) < 0 ) 
        exit(1);
    if ( lockf(pid_fd, F_TLOCK, 0) != 0) 
        exit(0);
    sprintf (buff, "%d\n", pid);
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

void * do_stuff (void * arg) {
    char buffer[256];
    int sckt = (int) arg;
     
    if ( write (sckt, "\n# ", 3) < 0) 
        error_and_exit("ERROR writing to socket\n");
      
    bzero (buffer, 256);
    if (read (sckt, buffer, 255) < 0) 
        error_and_exit("ERROR reading from socket\n");
   
    fprintf (stderr, "\n\tmessage: \'%s\'", buffer);
    if ( write(sckt, "From server:msg received", 24) < 0) 
        error_and_exit("ERROR writing to socket\n");  
    
    pthread_exit(NULL);
}

void error_and_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void log_time() {
    
    char out[20];
    time_t now;
    time(&now);

    struct tm * now_tm;
    now_tm = localtime(&now);

    strftime (out, 20, "\n%b %d %H:%M:%S ", now_tm);
    write (log_fd, out, strlen(out));   // OR   fprintf (stderr, "%s", out);
}

void reusePort(int s){
    int one=1;
    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one,sizeof(one)) == -1 )
        error_and_exit("ERROR: setsockopt -- SO_REUSEPORT\n");
} 

