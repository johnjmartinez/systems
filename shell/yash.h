#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

#define O_FOUT O_WRONLY|O_CREAT|O_TRUNC
#define O_FIN  O_RDONLY

// GLOBALS

typedef struct job {    
  struct job * next;    // next proc in pipe
  char * line;          // line
  pid_t cpgid;          // child group ID
  int done;             // 1 if process completed
  int paused;           // 1 if process stopped
  int status;           // reported status value
  int notified;
} job;
job * head_job;         // LL of jobs for accounting and signaling

pid_t yash_pgid, cgid;
int send_to_bg; // & in cmd line
int fwd, bck;   // fwd = out = fd for >; bck = in = fd for < 
int pipfd[2];   // | in cmd line

// looper.c
void yash_init();
void new_job (job * nj, char * line);

// tokenizer.c
bool tokenizer (char * line, char * _tokens[], int * count);

// parser.c
bool parser (char * _tokens[], int * pip, int * fwd, int * bck );
bool valid (int pip, int out, int in);     

// executor.c 
bool executor (char * cmds[], int pip, int out, int in, int count, job * j );
void exec_one (char * cmd[], job * j );
void exec_bck (char * cmd[], char * f_in, job * j );
void exec_fwd (char * cmd[], char * f_out, job * j );
void exec_in_out (char * cmd[], char * f_in, char * f_out, job * j );
void exec_in_pipe (char * cmd1[], char * cmd2[], char * f_in, job * j );
void exec_pipe (char * cmd1[], char * cmd2[], job * j );
void exec_pipe_out (char * cmd1[], char * cmd2[], char * f_out, job * j );
void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * f_in, char * f_out, job * j );

// jobatron.c
job * find_job (pid_t pgid);
int stopped_job (job * j);
int completed_job (job * j);
int mark_proc_status (pid_t pid, int status);
void update_status ();
void wait_for_job (job * j);
void print_job_info (job * j, const char * status);
void job_notify ();
