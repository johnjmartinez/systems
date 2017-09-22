/**
 * @file u-echod.c
 * @brief Daemon using Unix sockets. It just echoes back what
 *   it receives. It is an on-demand concurrent server with clients
 *  handled by separate tasks.
 *  Usage:
      % u-echod [<path_alt_to /tmp>]
 *  The name of the file associated with the socket is /tmp/u-echod
 *  The process id of the server is in the file /tmp/u-echod.pid
 *  The log file is /tmp/u-echod.log
 * This example is from Giorgio Ingargiola's Unix Programming Course
 * at Temple University 
 * (https://cis.temple.edu/~ingargio/cis307/readings/daemon.html)
 * I modified it a little bit and annotated it with explanations
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

extern int errno;

#define PATHMAX 255
static char u_server_path[PATHMAX+1] = "/tmp";  /* default */
static char u_socket_path[PATHMAX+1];
static char u_log_path[PATHMAX+1];
static char u_pid_path[PATHMAX+1];


/**
 * @brief  If we are waiting reading from a pipe and
 *  the interlocutor dies abruptly (say because
 *  of ^C or kill -9), then we receive a SIGPIPE
 *  signal. Here we handle that.
 */
void sig_pipe(int n) 
{
   perror("Broken pipe signal");
}


/**
 * @brief Handler for SIGCHLD signal 
 */
void sig_chld(int n)
{
  int status;

  fprintf(stderr, "Child terminated\n");
  wait(&status); /* So no zombies */
}


/**
 * @brief Initializes the current program as a daemon, by changing working 
 *  directory, umask, and eliminating control terminal,
 *  setting signal handlers, saving pid, making sure that only
 *  one daemon is running. Modified from R.Stevens.
 * @param[in] path is where the daemon eventually operates
 * @param[in] mask is the umask typically set to 0
 */
void daemon_init(const char * const path, uint mask)
{
  pid_t pid;
  char buff[256];
  static FILE *log; /* for the log */
  int fd;
  int k;

  /* put server in background (with init as parent) */
  if ( ( pid = fork() ) < 0 ) {
    perror("daemon_init: cannot fork");
    exit(0);
  } else if (pid > 0) /* The parent */
    exit(0);

  /* the child */

  /* Close all file descriptors that are open */
  for (k = getdtablesize()-1; k>0; k--)
      close(k);

  /* Redirecting stdin and stdout to /dev/null */
  if ( (fd = open("/dev/null", O_RDWR)) < 0) {
    perror("Open");
    exit(0);
  }
  dup2(fd, STDIN_FILENO);      /* detach stdin */
  dup2(fd, STDOUT_FILENO);     /* detach stdout */
  close (fd);
  /* From this point on printf and scanf have no effect */

  /* Redirecting stderr to u_log_path */
  log = fopen(u_log_path, "aw"); /* attach stderr to u_log_path */
  fd = fileno(log);  /* obtain file descriptor of the log */
  dup2(fd, STDERR_FILENO);
  close (fd);
  /* From this point on printing to stderr will go to /tmp/u-echod.log */

  /* Establish handlers for signals */
  if ( signal(SIGCHLD, sig_chld) < 0 ) {
    perror("Signal SIGCHLD");
    exit(1);
  }
  if ( signal(SIGPIPE, sig_pipe) < 0 ) {
    perror("Signal SIGPIPE");
    exit(1);
  }

  /* Change directory to specified directory */
  chdir(path); 

  /* Set umask to mask (usually 0) */
  umask(mask); 
  
  /* Detach controlling terminal by becoming sesion leader */
  setsid();

  /* Put self in a new process group */
  pid = getpid();
  setpgrp(); /* GPI: modified for linux */

  /* Make sure only one server is running */
  if ( ( k = open(u_pid_path, O_RDWR | O_CREAT, 0666) ) < 0 )
    exit(1);
  if ( lockf(k, F_TLOCK, 0) != 0)
    exit(0);

  /* Save server's pid without closing file (so lock remains)*/
  sprintf(buff, "%6d", pid);
  write(k, buff, strlen(buff));

  return;
}

/**
 * @brief In an infinite loop, it reads characters from sockfd and writes them
 * to the same socket
 * @param[in] socket descriptor to read and write
 */
void
chr_echo(int sockfd)
{
  for ( ; ; ) {
    ssize_t n;
    char c;
  
    n = read(sockfd, &c, 1);
    if (n > 0) {
       write(sockfd, &c, 1);
    } else if ( n < 0) { 
       perror("Negative return from Read");
       if (errno == EINTR) /* IO was interrupted by a signal */
           continue;
       return;
    } else  /* connection closed by other end */
      return;
  }
}

/**
 * @brief  Create and return a stream listening socket in the Unix
 *  domain using the filepath path.
 * @param[in] path for the UNIX domain socket 
 * @return the socket descriptor
 */
int u__listening_socket(const char *path)
{
    int fd;
    struct sockaddr_un  servaddr;
    
    if ( ( fd = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        exit(1);
    }
    
    bzero((void *)&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, path);
    
    if ( bind(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
        exit(1);
    }
    
    if ( listen(fd, 15) < 0 ) {
        exit(1);
    }
    return fd;
}

/**
 * @brief Return a connected socket obtain by an accept call
 *   on the listening socket lsd. It returns -1 if
 *  the accept was interrupted by a signal.
 * @param[in] lsd is the listening socket
 * @return the socket connected to
 */
int connected_socket(int lsd)
{
    int csd;
    size_t clilen;
    struct sockaddr_in cliaddr;
    
    clilen = sizeof(cliaddr);
    if ( (csd = accept(lsd, (struct sockaddr *) &cliaddr,
                       (int *)&clilen)) < 0) {
        if (errno == EINTR)
            return -1;
        else {
            exit(1);
        }
    }
    return csd;
}


int
main(int argc, char *argv[])
{
  int  listenfd;
  
  /* Initialize path variables */
  if (argc > 1) 
      strncpy(u_server_path, argv[1], PATHMAX); /* use argv[1] */
  strncat(u_server_path, "/", PATHMAX-strlen(u_server_path));
  strncat(u_server_path, argv[0], PATHMAX-strlen(u_server_path));
  strcpy(u_socket_path, u_server_path);
  strcpy(u_pid_path, u_server_path);
  strncat(u_pid_path, ".pid", PATHMAX-strlen(u_pid_path));
  strcpy(u_log_path, u_server_path);
  strncat(u_log_path, ".log", PATHMAX-strlen(u_log_path));
  
  daemon_init(u_server_path, 0); /* We stay in the u_server_path directory and file
                                    creation is not restricted. */
  
  unlink(u_socket_path); /* delete the socket if already existing */
  
  listenfd = u__listening_socket(u_socket_path);
  
  for ( ; ; ) {
      int connfd;
      pid_t childpid;
      
      if ( ( connfd = connected_socket(listenfd) ) < 0 ) continue;
    
      if ( ( childpid = fork() ) == 0) {        /* child process */
          close(listenfd);      /* close listening socket */
      chr_echo(connfd); /* process the request */
      exit(0);          /* terminate child */
      } else if (childpid < 0) {
          perror("Fork");
          exit(1);
      }
      close(connfd);      /* parent closes connected socket */
  }
  close(listenfd);
  return 0;
}

