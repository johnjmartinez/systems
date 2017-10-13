#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

#define O_FOUT O_WRONLY|O_CREAT|O_TRUNC
#define O_FIN  O_RDONLY

#define LOG_FILE "/tmp/yashd.log"
#define PID_FILE "/tmp/yashd.pid"
#define PORT_NUM 3826
#define MAX_CONNECTIONS 7

typedef struct job {
  struct job * next;    // next proc in pipe
  char * line;          // line
  pid_t cpgid;          // child proc group ID
  int done;             // 1 if process terminated
  int paused;           // 1 if process stopped
  int status;           // last reported status value
  int notify;           // 1 if status change
  int in_bg;
} job ;

typedef struct thread_stuff { 
    int tid;
    int sckt;
    int log_fd;
    struct sockaddr_in remote;
    job * head_job;
} t_stuff;

// daemon.c
pthread_t p[MAX_CONNECTIONS];
t_stuff t_data[MAX_CONNECTIONS];
pid_t cgid;             // current group id in fg

void d_init();          // daemon 
void s_init();          // socket/connection init
void shell_job ();
void * do_stuff (void * arg);
void error_n_exit(const char *msg);
void log_time(int fd);
void reuse_port(int s);

// tokenizer.c
bool tokenizer (char * line, char * _tokens[], int * count);
bool parser (char * _tokens[], int * pip, int * fwd, int * bck );
bool valid (int pip, int out, int in);

// executor.c
bool executor (char * cmds[], int pip, int out, int in, int count, char * line, t_stuff * data ); // waitpid
void exec_one (char * cmd[], job * j, int bg );
void exec_bck (char * cmd[], char * f_in, job * j, int bg );
void exec_fwd (char * cmd[], char * f_out, job * j, int bg );
void exec_in_out (char * cmd[], char * f_in, char * f_out, job * j, int bg );
void exec_in_pipe (char * cmd1[], char * cmd2[], char * f_in, job * j, int bg );
void exec_pipe (char * cmd1[], char * cmd2[], job * j, int bg );
void exec_pipe_out (char * cmd1[], char * cmd2[], char * f_out, job * j, int bg );
void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * f_in, char * f_out, job * j, int bg );

// jobatron.c
job * new_job (char * line, job * head);
char * get_status_str (job * j);
int running_job (job * j); 
job * find_bg_job (job * head);
job * find_fg_job (job * head);
job * find_job (pid_t pgid, job * head);
void job_list(job * head) ;
void job_list_all(job * head) ;
void job_notify (job * head);
void kill_jobs (job * head);
void log_job (pid_t pgid, job * j, int bg); // waitpid
void print_job_info (job * j, const char * status);
void update_status (job * head);
