#include "daemon.h"

int listen_fd, request_fd, pid_fd;
struct sockaddr_in host_addr, remote_addr;
static int avail[MAX_CONNECTIONS];
pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER; // OR pthread_mutex_init(&LOCK, NULL); ?

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
            error_n_exit("ERROR getpeername from socket");
        
        if ( getsockname (request_fd, (struct sockaddr *) &remote_addr, &len) < 0 )
            error_n_exit("ERROR getsockname from socket");

        /* DEBUG */
        //log_time();
        //fprintf (stderr, "NEW CONNECTION\t%s:%d:%d", inet_ntoa(remote_addr.sin_addr), 
        //    ntohs(remote_addr.sin_port), request_fd );

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

void s_init () {
    
    socklen_t size;
    log_time ();
    fprintf (stderr, "STARTING SERVER");

    listen_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( listen_fd < 0 )
 	    error_n_exit ("ERROR: s_init() -- starting socket");
    
    bzero ((void *) &host_addr, sizeof(host_addr)); // init to zeroes
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    host_addr.sin_port = htons(PORT_NUM);
    
    reuse_port (listen_fd);
    if ( bind(listen_fd, (struct  sockaddr *) &host_addr, sizeof(host_addr)) )
  	    error_n_exit("ERROR: s_init() -- binding socket");
       
    size = sizeof(host_addr);
    if ( getsockname (listen_fd, (struct sockaddr *) &host_addr, &size) ) 
	    error_n_exit("ERROR: s_init() -- getting socket name");
    
    fprintf(stderr, "\tport: %d", ntohs(host_addr.sin_port));
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
        error_n_exit("ERROR: d_init - can't open /dev/null");
    
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
            error_n_exit("ERROR writing to socket");
        
        skip = false;
        bzero (line, LINE_MAX);
        if (read (sckt_fd, line, LINE_MAX) < 0) 
            error_n_exit("ERROR reading from socket");
        
        count = 0;
        log_thread(line, data);
        tmp = strdup (line);
        
        skip = tokenizer (tmp, tokens, &count, sckt_fd);
        if ( skip || (tokens[0]==NULL) ) {
            free (tmp);
            log_thread(line, data);
            continue;
        }
        
        /* DEBUG */
        log_time();
        fprintf(stderr, "RECEIVED AT\t%s:%d:%d\t%s %s", data->ip_addr, data->port, data->s_fd,
                tmp, tokens[0]);
        
        if (strncmp(tokens[0], "CMD", 3) == 0 ) {  
             
            if (strncmp(tokens[1], "quit", 1) == 0)
                break;
            
            pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
            skip = parser (&tokens[1], &pipe_pos, &fwd_pos, &bck_pos); 
            if (skip) {
                free (tmp);
                log_thread(line, data);
                continue;
            }

            if (!valid (pipe_pos, fwd_pos, bck_pos)) {
                free (tmp);
                log_thread(line, data);
                continue;
            }
            
            executor (&tokens[1], pipe_pos, fwd_pos, bck_pos, count, line, data);
        }
        else if (strncmp(tokens[0], "CTL", 3) == 0) {
            
            if (strncmp(tokens[1], "c", 1) == 0)
                catch_c (data);
            else if (strncmp(tokens[1], "z", 1) == 0)
                catch_z (data);     
            else if (strncmp(tokens[1], "d", 1) == 0)
                break;
        }
        
        free (tmp);
    }

    avail[data->tid] = 2;
    close (sckt_fd);
    pthread_exit(NULL);
}

// ONLY APPLY TO FG (stopped or running) JOBS -- hence using cgid
void catch_c (t_stuff * data) { // CTL c
    job * i;
    char * err = "YASHD catch_c: cgid not found\n";

    if (data->cgid) {
        if ( (i = find_job(data->cgid, data->head_job)) == NULL )
            write (data->s_fd, err, strlen(err));
        else {
            i->done = 1;
            kill(data->cgid, SIGINT);
        }
    }
    write (data->s_fd, "\n# ", 3);
}

// ONLY APPLY TO FG (running) JOBS  -- hence using cgid
void catch_z (t_stuff * data) {   // CTL z
    job * i;
    char * err = "YASHD catch_z: cgid not found\n";
    
    if (data->cgid) {
        if ( (i = find_job (data->cgid, data->head_job)) == NULL )
            write (data->s_fd, err, strlen(err));
        else {
            i->paused = 1;
            kill(data->cgid, SIGTSTP);
        }
    }
    write (data->s_fd, "\n# ", 3);
}

void error_n_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void log_thread(char * line, t_stuff * data) { 
    // log template:
    // <date_n_time> yashd[<Client_IP>:<Port>]: <Command_Executed>
    char out[20];
    
    time_t now;
    time(&now);
    struct tm * now_tm;
    now_tm = localtime(&now);
    strftime (out, 20, "\n%b %d %H:%M:%S ", now_tm);
        
    pthread_mutex_lock(&LOCK);
    write (LOG_FD, out, strlen(out));   
    fprintf (stderr, "yashd[%s:%d:%d]: ", data->ip_addr, data->port, data->s_fd);
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

