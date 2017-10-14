#include "daemon.h"

int listen_fd, request_fd, pid_fd;
struct sockaddr_in host_addr, remote_addr;
static int avail[MAX_CONNECTIONS];

int main () {
    
    int i, served;
    
    d_init();   // daemon init
    s_init();   // socket init: socket(), bind(), listen()

    for (i = 0 ; i<MAX_CONNECTIONS; i++) {
        avail[i] = 1;
        t_data[i].tid = i;
    }
    
    socklen_t len = sizeof(remote_addr);
    for (;;) {
        request_fd = accept(listen_fd, (struct sockaddr *) &remote_addr, &len);
        
        if ( getpeername (request_fd, (struct sockaddr *) &remote_addr, &len) < 0 )
            error_n_exit("ERROR getpeername from socket\n");
        
        if ( getsockname (request_fd, (struct sockaddr *) &remote_addr, &len) < 0 )
            error_n_exit("ERROR getsockname from socket\n");

        log_time();
        fprintf (stderr, "NEW CONNECTION\t%s:%d", inet_ntoa(remote_addr.sin_addr), 
            ntohs(remote_addr.sin_port) );

        served = 0;
        for (i = 0 ; i<MAX_CONNECTIONS; i++) {
            if ( (avail[i] == 1) && !served) {
                t_data[i].s_fd = request_fd;
                t_data[i].ip_addr = inet_ntoa(remote_addr.sin_addr);
                t_data[i].port = ntohs(remote_addr.sin_port);
                avail[i] = 0;
                served = 1;
                pthread_create (&p[i], NULL, shell_job, &t_data[i]);
            }
            if (avail[i] == 2) {
                pthread_join(p[i], NULL);
                avail[i] = 1;
            }
        }
    }
    
    for (i = 0 ; i<MAX_CONNECTIONS; i++)
        pthread_join(p[i], NULL);

    close(listen_fd);
    close(pid_fd);
    
    log_time();
    fprintf (stderr, "CLOSING SERVER");
    close(LOG_FD);

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
    log_time ();
    fprintf (stderr, "STARTING SERVER ");

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
    
    dup2 (fd, STDIN_FILENO);            // redirect STDIN  to /dev/null
    dup2 (fd, STDOUT_FILENO);           // redirect STDOUT to /dev/null
    close (fd);

    log_file = fopen (LOG_FILE, "aw");  // open: write + append     
    LOG_FD = fileno (log_file);         // get GLOBAL log file descriptor
    dup2 (LOG_FD, STDERR_FILENO);       // redirect STDERR to log

    /* TODO -- SIGCHLD & SIGPIPE
    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_n_exit("Signal SIGCHLD");
    if ( signal(SIGPIPE, sig_pipe) < 0 ) 
        error_n_exit("Signal SIGPIPE");
     */
    
    //umask(0);                         // implicit declaration of function ‘umask'. SO???
    chdir("/");
    pid = setsid();                     // put self in new process group 
    setpgrp();        

    // Save server's pid; make sure ONLY ONE yashd server running 
    if ( (pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0666)) < 0 ) 
        exit(1);
    if (lockf(pid_fd, F_TLOCK, 0) != 0) 
        exit(0);
    sprintf (buff, "%d\n", pid);
    write (pid_fd, buff, strlen(buff));
}

void * shell_job (void * arg) {    
    
    t_stuff * data = (t_stuff *) arg;
    data->head_job = NULL;
    int sckt_fd = data->s_fd;

    char line[LINE_MAX];
    char * tokens[LINE_MAX/3];
    char * tmp;
    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;
    
    for(;;) {
        job_notify (data->head_job);
        if ( write (sckt_fd, "\n# ", 3) < 0) 
            error_n_exit("ERROR writing to socket\n");
        
        skip = false;
        bzero (line, LINE_MAX);
        if (read (sckt_fd, line, LINE_MAX) < 0) 
            error_n_exit("ERROR reading from socket\n");

        tmp =  strdup (line);
        count = 0;
        skip = tokenizer (tmp, tokens, &count);
        //if ( skip || (tokens[0]==NULL) ) {
        //    free (tmp);
        //    continue;
        //}
        
        // TODO -- check CTL cmds and create handlers
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
        
        log_thread(line, data);
        executor (tokens, pipe_pos, fwd_pos, bck_pos, count, line, data);
        free (tmp);
    }
    
    avail[data->tid] = 2;
    close (sckt_fd);
    pthread_exit(NULL);
}

void error_n_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void log_thread(char * line, t_stuff * data) { 
    // log template:
    // "<date_n_time> yashd[<Client_IP>:<Port>]: <Command_Executed>"
    
    char out[20];
    
    time_t now;
    time(&now);
    struct tm * now_tm;
    now_tm = localtime(&now);
    strftime (out, 20, "\n%b %d %H:%M:%S ", now_tm);
        
    pthread_mutex_lock(&LOCK);
    write (LOG_FD, out, strlen(out));   
    fprintf (stderr, "yashd[%s:%d]: ", data->ip_addr, data->port);
    write (LOG_FD, line, strlen(line)-1);   
    pthread_mutex_unlock(&LOCK);

}

void log_time() { 
    
    char out[20];
    
    time_t now;
    time(&now);
    struct tm * now_tm;
    now_tm = localtime(&now);
    strftime (out, 20, "\n%b %d %H:%M:%S ", now_tm);
    write (LOG_FD, out, strlen(out));   

}

void reuse_port(int s ){
    int one=1;
    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) == -1 )
        error_n_exit("ERROR: setsockopt -- SO_REUSEPORT\n");
} 

