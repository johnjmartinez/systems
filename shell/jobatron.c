#include "yash.h"

// The following is heavily influenced by
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * new_job (char * line) {

        job * nj = (job *) malloc ( sizeof (job) );
        nj->line = line ;
        nj->cpgid = 0;
        nj->paused = 0;
        nj->status = 0;
        nj->notified = 0;
        nj->in_bg = 0;
        nj->next = head_job;

        /*job * curr = head_job;
        if (curr != NULL) {
            while (curr->next != NULL)
                curr = curr->next;
            curr->next = nj;
        }
        else*/
        head_job = nj; // front of list

        return nj;
}

void log_job (pid_t pgid, job * j) {
    int status;
    j->cpgid = pgid;

    if (!send_to_bg ) {
        cgid = pgid;
        waitpid(pgid, &status, WUNTRACED );
        j->status = status;
    }
    else {
        j->in_bg = 1;
        waitpid(pgid, &status, WUNTRACED | WNOHANG );
        j->status = status;
    }
}

void kill_jobs () {

    job * target;
    job * curr = head_job;
    if (curr != NULL) {
        while (curr->next != NULL) {
            target = curr;
            curr = curr->next;
            kill(- target->cpgid, SIGINT);
            free(target);
        }
        kill(- curr->cpgid, SIGINT);
        free(curr);
    }
}

job * find_fg_job () {  // Find newest job->paused/in_bg

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused || j->in_bg) return j;

    return NULL;
}

job * find_bg_job () {  // Find newest job->paused

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused) return j;

    return NULL;
}

void job_list() {
    job * j;
    for (j = head_job; j; j = j->next) {

        //fprintf(stdin, "[%d] -  %s\t%s", j->cpgid, status_update(j), j->line);
        fprintf(stdin, "[%d] -  %d\t%s", j->cpgid, j->status, j->line);
        
    }
}

char * status_update (job * j) {

    int status = j->status;
    if (WIFSTOPPED (status)) {
        j->paused = 1;
        return "Stopped";
    }
    j->done = 1;
    if (WIFSIGNALED (status))
        fprintf (stderr, "%d: killed by %d\n", j->cpgid, WTERMSIG (j->status));

    return "Done   ";
}


job * find_job (pid_t pgid) {       // Find active job using pgid.

  job * j;
  for (j = head_job; j; j = j->next)
    if (j->cpgid == pgid) return j;

  return NULL;
}

int paused_job (job * j) {       // 1 if job's paused or done.

    if (!j->done && !j->paused) return 0;
    return 1;
}

int done_job (job * j) {     // 1 if job's done.
    return j->done;
}

int mark_job_status (pid_t pid, int status) {

    job * j;
    if (pid > 0) {  // Update proc record
        for (j = head_job; j; j = j->next) {
            if (j->cpgid == pid) {

                j->status = status;
                if (WIFSTOPPED (status))
                    j->paused = 1;
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
    } while (!mark_job_status (pid, status));
}

void wait_for_job (job * j) {
    int status;
    pid_t pid;

    do {
        pid = waitpid (WAIT_ANY, &status, WUNTRACED);
    } while (!mark_job_status (pid, status) && !paused_job (j) && !done_job (j));
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
