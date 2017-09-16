#include "yash.h"

// The following is heavily influenced by 
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * find_job (pid_t pgid) {       // Find active job using pgid. 
  
  job * j;
  for (j = head_job; j; j = j->next)
    if (j->cpgid == pgid) return j;
      
  return NULL;
}

int paused_job (job * j) {       // 1 if all jobs have paused or done.
  
  for (j = head_job; j; j = j->next)
    if (!j->done && !j->paused) return 0;
      
  return 1;
}

int done_job (job * j) {     // 1 if all jobs have done. 
 
  for (j = head_job; j; j = j->next)
    if (!j->done)
      return 0;
      
  return 1;
}


int mark_proc_status (pid_t pid, int status) {

    job * j;
    
    if (pid > 0) {  // Update proc record 
        for (j = head_job; j; j = j->next) {
            if (j->cpgid == pid) {
            
                j->status = status;
                if (WIFSTOPPED (status)) j->paused = 1;
                else {
                    j->done = 1;
                    if (WIFSIGNALED (status))
                    fprintf (stderr, "%d: killed by %d\n", (int) pid, WTERMSIG (j->status));
                }
                return 0;
            }
        }
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
    } while (!mark_proc_status (pid, status) && !paused_job (j) && !done_job (j));
}

void print_job_info (job * j, const char * status) {
  fprintf (stderr, "%ld (%s): %s\n", (long)j->cpgid, status, j->line);
}

void job_notify () {

    job * j, * jlast, * jnext;

    update_status ();   // Update status information for child procs
    
    jlast = NULL;
    for (j = head_job; j; j = jnext) {
        jnext = j->next;

        if (done_job (j)) {                         // notify about & delete done job
            print_job_info (j, "done");
            if (jlast) jlast->next = jnext;
            else  head_job = jnext;
            free (j);
        }
        else if (paused_job (j) && !j->notified) {  // notify about paused job   
            print_job_info (j, "paused");
            j->notified = 1;
            jlast = j;
        }
        else                                        // running job
            jlast = j;
    }
}
