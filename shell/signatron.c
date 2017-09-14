#include "yash.h"

// The following heavily influenced by 
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * find_job (pid_t pgid) {       // Find active job using pgid. 
  
  job * j;
  for (j = head_job; j; j = j->next)
    if (j->pgid == pgid)
      return j;
      
  return NULL;
}

int stopped_job (job *j) {       // 1 if all procs in job have stopped or completed.
  
  process * p;
  for (p = j->head_proc; p; p = p->next)
    if (!p->completed && !p->stopped)
      return 0;
      
  return 1;
}

int completed_job (job *j) {     // 1 if all procs in job have completed. 
 
  process * p;
  for (p = j->head_proc; p; p = p->next)
    if (!p->completed)
      return 0;
      
  return 1;
}

void launch_job (proc * p, pid_t pgid, int fg) {

    pid_t pid = getpid ();
    if (pgid == 0) pgid = pid;
    setpgid (pid, pgid);
    if (fg) tcsetpgrp (STDIN_FILENO, pgid);

    /* Set the handling for job control signals back to the default.  */
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);


    /* Exec the new process.  Make sure we exit.  */
    execvp (p->cmd[0], p->cmd);
    perror ("YAHS");
    exit (1);
}
