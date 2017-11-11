#include "daemon.h"

int listen_fd, request_fd, pid_fd;
struct sockaddr_in host_addr, remote_addr;
static int avail[MAX_CONNECTIONS];
pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER; // OR pthread_mutex_init(&LOCK, NULL); ?

int main () {
    
    signal(SIGPIPE, SIG_IGN);
   
    d_init();   // daemon init
    s_init();   // socket init: socket(), bind(), listen()
    
    int i, served;
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

    /* TODO -- SIGCHLD 
    if ( signal(SIGCHLD, sig_chld) < 0 ) 
        error_n_exit("Signal SIGCHLD");
    */
    
    //umask(0);                         // gives: implicit declaration of function ‘umask'. SO???
    chdir("/");
    pid = setsid();                     // put self in new process group 
    setpgrp();        

    // Save server's pid; make sure ONLY ONE yashd server running 
    if ( (pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0666)) < 0 ) 
        exit(1);
    if (lockf(pid_fd, F_TLOCK, 0) != 0) 
        exit(0);
    snprintf (buff, 16, "%d\n", pid);
    write (pid_fd, buff, strlen(buff));
}

void * shell_job (void * arg) {    
        
    t_stuff * data = (t_stuff *) arg;
    data->head_job = NULL;
    data->write_ok = true;
    int sckt_fd = data->s_fd;    

    char line[LINE_MAX];
    char * tokens[LINE_MAX/3];
    char * tmp;
    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip, in_ready;
    pthread_t auxiliar;

    log_thread ("STARTING CONNECTION ", data);

    t_incoming * in = (t_incoming *) malloc ( sizeof (t_incoming) );
    in->data = data;
    in->ready = false;
    in->sckt_fd = sckt_fd;
    pthread_mutex_init(&in->check_lock, NULL);
    pthread_create (&auxiliar, NULL, aux_listen, in);
    
    for(;;) {
        
        if (in->close)
                break;
        job_notify (data); // contains write to socket

        skip = false;
        bzero (line, LINE_MAX);
        
        pthread_mutex_lock (&in->check_lock);
          in_ready = in->ready;
          if (in_ready) {
            count = in->num_read;
            strcpy (line, in->in_buffer);
            in->ready = false;
          }    
        pthread_mutex_unlock (&in->check_lock);
        
        if (!in_ready || !count)
            continue;
        
        log_thread (line, data);
        tmp = strdup (line);
        tokenizer (tmp, tokens, &count, sckt_fd);   // count will be converted to tokens num

        if (strncmp(tokens[0], "CMD", 3) == 0 ) {  
            
            for (int x=1; x <= count; x++)          // get rid of first token (CMD)
                tokens[x-1] = tokens[x];
            
            pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
            skip = parser (tokens, &pipe_pos, &fwd_pos, &bck_pos, sckt_fd); 
            if ( skip || !valid (pipe_pos, fwd_pos, bck_pos, sckt_fd) ) {
                free (tmp);
                data->write_ok = true;
                continue;
            }
            
            executor (tokens, pipe_pos, fwd_pos, bck_pos, count-1, line, data);
        }
        free (tmp);
        sleep(.7);
        data->write_ok =true;
    }
    log_thread ("CLOSING CONNECTION ", data);
        
    close (sckt_fd);
    avail[data->tid] = 2;
    pthread_exit(NULL);
}

// ONLY APPLY TO FG (stopped or running) JOBS
void catch_c (t_stuff * data) { // CTL c
    job * i;
    char * err = "YASHD catch_c: cgid not found\n";

    if (data->cgid) {
        if ( (i = find_job(data->cgid, data->head_job)) == NULL )
            write (data->s_fd, err, strlen(err));
        else {
            i->done = 1;
            data->write_ok = true;
            kill(data->cgid, SIGINT);
        }
    }
}

// ONLY APPLY TO FG (running) JOBS
void catch_z (t_stuff * data) {   // CTL z
    job * i;
    char * err = "YASHD catch_z: cgid not found\n";
    
    if (data->cgid) {
        if ( (i = find_job (data->cgid, data->head_job)) == NULL )
            write (data->s_fd, err, strlen(err));
        else {
            i->paused = 1;
            write (data->s_fd, "\n# ", 3);
            kill(data->cgid, SIGTSTP);
        }
    }
}

void error_n_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void log_thread(char * line, t_stuff * data) { 
    // log template: <date_n_time> yashd[<Client_IP>:<Port>]: <Command_Executed>
    char time_out[20];
    char str_out[50];
    
    time_t now;
    time(&now);
    struct tm * now_tm;
    now_tm = localtime(&now);
    
    strftime (time_out, 20, "\n%b %d %H:%M:%S ", now_tm);
    snprintf (str_out, 50, "yashd[%s:%d:%d]: ", data->ip_addr, data->port, data->s_fd);
        
    pthread_mutex_lock(&LOCK);
    write (LOG_FD, time_out, strlen(time_out));   
    write (LOG_FD, str_out, strlen(str_out));   
    write (LOG_FD, line, strlen(line)-1);   // get rid of line last char ... usually \n
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
    int one = 1;
    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) == -1 )
        error_n_exit("ERROR: setsockopt -- SO_REUSEPORT\n");
} 

void * aux_listen (void * incoming) { // AUXILIARY THREAD
    
    char buf[LINE_MAX];
    char * tokens[LINE_MAX/3];
    char * tmp;
    int count, num;
    t_incoming * in = (t_incoming *) incoming;
    bool skip;

    for (;;) {
        bzero(buf, LINE_MAX);
        
        if (in->ready)
            continue;
        else if ( (num = read (in->sckt_fd, buf, LINE_MAX)) < 0 ) {
            log_thread ("ERROR: receiving client msg ", in->data);
            break;
        }
        else if (num) {
            
            buf[num]='\0';
            tmp = strdup (buf);

            skip = tokenizer (tmp, tokens, &count, in->sckt_fd); // count = tokens num
            if ( skip || (tokens[0]==NULL) ) {
                free (tmp);
                continue;
            }
            
            if (strncmp(tokens[0], "CTL", 3) == 0) {
                log_thread (buf, in->data);
            
                if (strncmp(tokens[1], "c", 1) == 0)
                    catch_c (in->data);
                else if (strncmp(tokens[1], "z", 1) == 0)
                    catch_z (in->data);     
                else if (strncmp(tokens[1], "d", 1) == 0) {
                    in->close = true;
                    break;
                }
                
                free (tmp);
                continue;
            }
            
            free (tmp);

            pthread_mutex_lock (&in->check_lock);
              strcpy (in->in_buffer, buf);
              in->num_read = num;
              in->ready = true;
            pthread_mutex_unlock (&in->check_lock);
        }
        else {    
            log_thread ("ERROR: Disconnected ", in->data);
            in->close = true;
            break;
        }
    }
    
    close (in->sckt_fd);
    pthread_exit(NULL);
}
