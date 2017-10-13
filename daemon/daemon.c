#include "daemon.h"

int listen_fd, request_fd, pid_fd, log_fd;
struct sockaddr_in host_addr, remote_addr;
static int avail[MAX_CONNECTIONS];

int main () {
    
    int i, served;
    
    d_init();   // daemon init
    s_init();   // socket init: socket(), bind(), listen()

    for (i = 0 ;  i<MAX_CONNECTIONS; i++) {
        avail[i] = 1;
        t_data[i].tid = i;
        t_data[i].log_fd = log_fd;
    }
    
    socklen_t len = sizeof(remote_addr);
    //for (;;) {
        request_fd = accept(listen_fd, (struct sockaddr *) &remote_addr, &len);
        
        log_time(log_fd);
        fprintf (stderr, "NEW CONNECTION at %6d", request_fd);

        served = 0;
        for (i = 0 ;  i<MAX_CONNECTIONS; i++) {
            if ( (avail[i] == 1) && !served) {
                t_data[i].sckt = request_fd;
                t_data[i].remote = remote_addr;
                avail[i] = 0;
                served = 1;
                pthread_create (&p[i], NULL, do_stuff, &t_data[i]);
            }
            if (avail[i] == 2) {
                pthread_join(p[i], NULL);
                avail[i] = 1;
            }
        }
    //}
    
    for (i = 0 ;  i<MAX_CONNECTIONS; i++)
        pthread_join(p[i], NULL);

    close(listen_fd);
    close(pid_fd);
    
    log_time(log_fd);
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
        }
    }
    fprintf(stdout, "\n");
}*/

void s_init () {
    
    socklen_t size;
    log_time (log_fd);
    fprintf (stderr, "STARTING SERVER ... ");

    listen_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( listen_fd < 0 )
 	    error_n_exit ("ERROR: s_init() -- starting socket\n");
    
    bzero ((void *) &host_addr, sizeof(host_addr)); // init to zeroes
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    host_addr.sin_port = htons(PORT_NUM);
    
    reuse_port (listen_fd);
    if ( bind(listen_fd, (struct  sockaddr *) &host_addr, sizeof(host_addr)) )
  	    error_n_exit("ERROR: s_init() -- binding socket\n");
       
    size = sizeof(host_addr);
    if ( getsockname (listen_fd, (struct sockaddr *) &host_addr, &size) ) 
	    error_n_exit("ERROR: s_init() -- getting socket name\n");
    
    fprintf(stderr, "at port: %d", ntohs(host_addr.sin_port));
    listen ( listen_fd, MAX_CONNECTIONS ); 
}

void d_init() {
    
    int fd;
    pid_t pid;
    static FILE * log_file;
    char buff[16];

    if ( (pid = fork()) < 0 ) 
        error_n_exit("ERROR: d_init - cannot fork\n");
    else if (pid) 
        exit(0);                        // PARENT EXITS - server to background

    for (fd = getdtablesize()-1; fd>0; fd--) 
        close(fd);                      // close any open fds

    if ( (fd = open("/dev/null", O_RDWR)) < 0) 
        error_n_exit("ERROR: d_init - can't open /dev/null\n");
    dup2 (fd, STDIN_FILENO);            // redirect STDIN to /dev/null
    dup2 (fd, STDOUT_FILENO);           // redirect STDOUT to /dev/null
    close (fd);

    log_file = fopen (LOG_FILE, "aw");  // open: write + append     
    log_fd = fileno (log_file);         // get log file descrpt
    dup2 (log_fd, STDERR_FILENO);       // redirect STDERR to log

    /* TODO -- SIGCHLD & SIGPIPE
    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_n_exit("Signal SIGCHLD");
    if ( signal(SIGPIPE, sig_pipe) < 0 ) 
        error_n_exit("Signal SIGPIPE");
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
    
    //head_job = NULL;    // LL of jobs for accounting and signaling

    char line[LINE_MAX];
    char * tokens[LINE_MAX/3];
    char * tmp;

    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;
    
    for(;;) {
    
        //job_notify();
        fflush(stdout);
       
        fprintf(stdout, "\n# ");
        fflush(stdout);
        skip = false;

        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            //kill_jobs();
            return;
        }

        tmp =  strdup (line);
        count = 0;

        skip = tokenizer (tmp, tokens, &count);
        if ( skip || (tokens[0]==NULL) ) {
            free (tmp);
            continue;
        }

        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser (tokens, &pipe_pos, &fwd_pos, &bck_pos);
        if (skip) {
            free (tmp);
            continue;
        }

        if (!valid (pipe_pos, fwd_pos, bck_pos)) {
            free (tmp);
            continue;
        }

        //executor (tokens, pipe_pos, fwd_pos, bck_pos, count, line);
        free (tmp);
    }
}

void * do_stuff (void * arg) {
    
    t_stuff * data = (t_stuff *) arg;
    data->head_job = NULL;

    int pipe_pos, fwd_pos, bck_pos, count;
    char line[LINE_MAX];
    char * tokens[LINE_MAX/3];
    char * tmp;
    
    int sckt = data->sckt;
    if ( write (sckt, "\n# ", 3) < 0) 
        error_n_exit("ERROR writing to socket\n");
      
    bzero (line, LINE_MAX);
    if (read (sckt, line, LINE_MAX) < 0) 
        error_n_exit("ERROR reading from socket\n");
    
    tmp =  strdup (line);
    count = 0;
    tokenizer (tmp, tokens, &count);

    pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
    parser (tokens, &pipe_pos, &fwd_pos, &bck_pos); 
    
    log_time(data->log_fd);
    fprintf (stderr, "message: \'%s\'", line); 
    
    executor (tokens, pipe_pos, fwd_pos, bck_pos, count, line, data);
    
    avail[data->tid] = 2;
    close (sckt);
    pthread_exit(NULL);
}

void error_n_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void log_time(int fd) { 
    
    char out[20];
    time_t now;
    time(&now);

    struct tm * now_tm;
    now_tm = localtime(&now);

    strftime (out, 20, "\n%b %d %H:%M:%S ", now_tm);
    write (fd, out, strlen(out));   // OR   fprintf (stderr, "%s", out);
}

void reuse_port(int s ){
    int one=1;
    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) == -1 )
        error_n_exit("ERROR: setsockopt -- SO_REUSEPORT\n");
} 

