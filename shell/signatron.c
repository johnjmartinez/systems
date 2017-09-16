#include "yash.h"


void my_wait(int _gid) { 

    int status;
    while (1) {

        if ( (waitpid(- _gid, &status, WUNTRACED | WCONTINUED )) < 0 ) {
            perror("wait_gid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("child %d exited, status=%d\n", _gid, WEXITSTATUS(status)); 
            break;
        }
        else if (WIFSIGNALED(status)) {
            printf("child %d killed by signal %d\n", _gid, WTERMSIG(status)); 
            break;
        }
        else if (WIFSTOPPED(status)) {
            printf("%d stopped by signal %d\n", _gid, WSTOPSIG(status));
            break;
        } 
        else if (WIFCONTINUED(status)) 
            printf("Continuing %d\n", _gid); // continue blocking
    }

}



// The following is heavily influenced by 
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * find_job (pid_t pgid) {       // Find active job using pgid. 
  
  job * j;
  for (j = head_job; j; j = j->next)
    if (j->pgid == pgid) return j;
      
  return NULL;
}

int stopped_job (job * j) {       // 1 if all procs in job have stopped or completed.
  
  proc * p;
  for (p = j->head_proc; p; p = p->next)
    if (!p->completed && !p->stopped) return 0;
      
  return 1;
}

int completed_job (job * j) {     // 1 if all procs in job have completed. 
 
  proc * p;
  for (p = j->head_proc; p; p = p->next)
    if (!p->completed)
      return 0;
      
  return 1;
}

void launch_proc (proc * p, pid_t pgid, int fg) {

    if (fg) tcsetpgrp (STDIN_FILENO, pgid);

    //signal (SIGINT,  SIG_DFL); // Set handling job control signals back to default
    signal (SIGQUIT, SIG_DFL);
    //signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);

    execvp (p->cmd[0], p->cmd); // Exec the new proc and exit. 
    perror ("YASH");
    exit (1);
}

void launch_job (job * j, int fg) {

    proc * p;
    pid_t pid;

    for (p = j->head_proc; p; p = p->next) {

        pid = fork ();
        
        if (pid == 0) 
            launch_proc (p, j->pgid, fg);
        else {
            p->pid = pid;
            if (!j->pgid) j->pgid = pid;
            setpgid (pid, j->pgid);
        }
    }

    print_job_info (j, "launched");

    if (fg) mv_job_to_fg (j, 0);
    else    mv_job_to_bg (j, 0);
}

void mv_job_to_fg (job * j, int cont) {
    
    tcsetpgrp (STDIN_FILENO, j->pgid);  // Put job into fg
    
    if (cont) {         // Send job continue signal if necessary
        tcsetattr (STDIN_FILENO, TCSADRAIN, &j->modes);
        if (kill (- j->pgid, SIGCONT) < 0) perror ("kill (SIGCONT)");
    }

    wait_for_job (j);   // Wait for it to report. 

    tcsetpgrp (STDIN_FILENO, yash_pgid); // Put shell back in fg. 
    tcgetattr (STDIN_FILENO, &j->modes);  // Restore shell's terminal modes.
    tcsetattr (STDIN_FILENO, TCSADRAIN, &yash_modes);
}


void mv_job_to_bg (job * j, int cont) {
  
  if (cont)         // Send job continue signal if necessary 
    if (kill (-j->pgid, SIGCONT) < 0) perror ("kill (SIGCONT)");
}

int mark_proc_status (pid_t pid, int status) {

    job * j;
    proc * p;

    if (pid > 0) {  // Update proc record 
        for (j = head_job; j; j = j->next) {
            for (p = j->head_proc; p; p = p->next) {
                if (p->pid == pid) {
                
                    p->status = status;
                    if (WIFSTOPPED (status)) p->stopped = 1;
                    else {
                        p->completed = 1;
                        if (WIFSIGNALED (status))
                        fprintf (stderr, "%d: Terminated by %d\n", (int) pid, WTERMSIG (p->status));
                    }
                    return 0;
        }   }   }
        fprintf (stderr, "No child proc %d.\n", pid);
        return -1;
    }
    else if (pid == 0 || errno == ECHILD)
        return -1;  // No procs ready to report 
    else {
        perror ("waitpid");
        return -1;
    }
}

void update_status () {
    int status;
    pid_t pid;

    do {
        pid = waitpid (WAIT_ANY, &status, WUNTRACED|WNOHANG);
    } while (!mark_proc_status (pid, status));
}

void wait_for_job (job * j) {
    int status;
    pid_t pid;

    do {
        pid = waitpid (WAIT_ANY, &status, WUNTRACED);
    } while (!mark_proc_status (pid, status) && !stopped_job (j) && !completed_job (j));
}

void print_job_info (job * j, const char * status) {
  fprintf (stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->cmd);
}

void job_notify () {

    job * j, * jlast, * jnext;

    update_status ();   // Update status information for child procs
    
    jlast = NULL;
    for (j = head_job; j; j = jnext) {
        jnext = j->next;

        if (completed_job (j)) {                    // notify about & delete completed job
            print_job_info (j, "completed");
            if (jlast) jlast->next = jnext;
            else  head_job = jnext;
            free (j);
        }
        else if (stopped_job (j) && !j->notified) { // notify about stopped job   
            print_job_info (j, "stopped");
            j->notified = 1;
            jlast = j;
        }
        else                                        // running job
            jlast = j;
    }
}

void continue_job (job * j, int fg) {

    proc * p;
    for (p = j->head_proc; p; p = p->next)  // mark job running              
        p->stopped = 0;
        
    j->notified = 0;
    
    if (fg) mv_job_to_fg (j, 1);
    else mv_job_to_bg (j, 1);
}
