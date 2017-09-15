#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

#define O_FOUT O_WRONLY|O_CREAT|O_TRUNC
#define O_FIN  O_RDONLY

void yash_init();

bool tokenizer (char * line, char * _tokens[], int * count);

bool parser (char * _tokens[], int * pip, int * fwd, int * bck );
bool valid (int pip, int out, int in);     

bool executor (char * _tokens[], int pip, int out, int in, int count );

void exec_bck (char * cmd[], char * filein);
void exec_fwd (char * cmd[], char * fileout);
void exec_in_out (char * cmd[], char * filein, char * fileout);
void exec_in_pipe (char * cmd1[], char * cmd2[], char * filein);
void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * filein, char * fileout);
void exec_one (char * _tokens[]);
void exec_pipe(char * cmd1[], char * cmd2[]);
void exec_pipe_out (char * cmd1[], char * cmd2[], char * fileout);

// The following heavily influenced by 
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

typedef struct proc  {      // PROCESS = single cmd + args + redirects 
  struct proc * next;         // next proc in pipe
  char ** cmd;                // for exec -- same as char * cmd[]
  pid_t pid;                  // process ID
  int completed;              // 1 if process completed
  int stopped;                // 1 if process stopped
  int status;                 // reported status value
} proc;

typedef struct job {        // JOB = pipeline of processes. 
  struct job * next;          // next active job
  char * command;             // command line, used for messages
  proc * head_proc;           // LL of procs in job
  pid_t pgid;                 // process group ID
  int notified;               // 1 if notified about stopped job
  struct termios modes;       // saved terminal modes
} job;

job * find_job (pid_t pgid);
int stopped_job (job *j);
int completed_job (job *j);
void launch_job (proc * p, pid_t pgid, int fg);


