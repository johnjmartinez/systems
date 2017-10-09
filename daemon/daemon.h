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

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

#define O_FOUT O_WRONLY|O_CREAT|O_TRUNC
#define O_FIN  O_RDONLY

#define LOG_FILE "/tmp/yashd.log"

// daemon.c

void d_init();          // daemon 
void s_init();          // socket/connection init
void shell_job ();
void error_and_exit(const char *msg) ;

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

job * head_job;         // LL of jobs for accounting and signaling

pid_t yash_pgid, cgid;  // current group id in fg
int send_to_bg;         // in cmd line
int fwd, bck;           // fwd = out = fd for >; bck = in = fd for <
int pipfd[2];           // | in cmd line

// tokenizer.c
bool tokenizer (char * line, char * _tokens[], int * count);
bool parser (char * _tokens[], int * pip, int * fwd, int * bck );
bool valid (int pip, int out, int in);

// executor.c
bool executor (char * cmds[], int pip, int out, int in, int count, char * line ); // waitpid

void exec_one (char * cmd[], job * j );
void exec_bck (char * cmd[], char * f_in, job * j );
void exec_fwd (char * cmd[], char * f_out, job * j );
void exec_in_out (char * cmd[], char * f_in, char * f_out, job * j );
void exec_in_pipe (char * cmd1[], char * cmd2[], char * f_in, job * j );
void exec_pipe (char * cmd1[], char * cmd2[], job * j );
void exec_pipe_out (char * cmd1[], char * cmd2[], char * f_out, job * j );
void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * f_in, char * f_out, job * j );

// jobatron.c
job * new_job (char * line);

char * get_status_str (job * j);
int running_job (job * j); 

job * find_bg_job ();
job * find_fg_job ();
job * find_job (pid_t pgid);

void job_list() ;
void job_list_all() ;
void job_notify ();
void kill_jobs ();
void log_job (pid_t pgid, job * j); // waitpid
void print_job_info (job * j, const char * status);
void update_status ();
