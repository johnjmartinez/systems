#include "yash.h"

// The following is heavily influenced by
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * new_job (char * line) {

        line[strlen(line)-1] = '\0';    // get rid of '\n'
        job * nj = (job *) malloc ( sizeof (job) );
        nj->line = strdup(line);
        nj->cpgid = 0;
        nj->paused = 0;
        nj->status = 0;
        nj->in_bg = 0;
        nj->next = head_job;

        head_job = nj;                  // front of list (push)
        return nj;
}

void log_job (pid_t pgid, job * j) {
    int status;
    j->cpgid = pgid;

    if (!send_to_bg ) {
        cgid = pgid;
        waitpid (pgid, &status, WUNTRACED ); 
        j->status = status;     
        // /*DEBUG*/ printf("--: status %d of %d\n", status, pgid);
    }
    else {
        j->in_bg = 1;
    }
}

void job_notify () {                    // a.k.a. 'jobs'  

    job * j, * jprev, * jnext;

    update_status ();                   // Update status info for jobs

    jprev = NULL;
    for (j = head_job; j; j = jnext) {
        jnext = j->next;

        if (j->done) {                  // notify about & delete done job
        
            if (j->in_bg)  
                 print_job_info (j, "Done   ");
                 
            /* -- can't find bug with marking jobs
            if (jprev) 
                jprev->next = jnext;
            else  
                head_job = jnext;
            free (j);*/
        }
        else                                        
            jprev = j;
    }
}

void update_status () {

    int status;
    job * j;
    for (j = head_job; j; j = j->next) {
    
        waitpid (j->cpgid, &status, WUNTRACED | WCONTINUED | WNOHANG); 
        j->status = status;
        
        //if ( WIFSTOPPED (status) && !WIFCONTINUED (status) ) j->paused = 1;
        if ( WIFEXITED (status) || WIFSIGNALED (status) ) 
            if(!j->paused) j->done = 1;
        // /*DEBUG*/ printf("-update: status %d of %d\n", status, j->cpgid);
    }
}

void job_list() {
    job * j;
    update_status ();   // update status info for jobs

    for (j = head_job; j; j = j->next) {
        if (j->in_bg) 
            print_job_info(j, get_status_str(j));
    }
}

void job_list_all() {
    job * j;
    fprintf(stdout, "DEBUG\n");
    for (j = head_job; j; j = j->next) {
        print_job_info(j, get_status_str(j));
    }
}

char * get_status_str (job * j) {

    if (WIFSTOPPED (j->status)) return "Stopped";
    return "Running";
}


int running_job (job * j) {     // 1 if job's running 

    if (!j->done && !j->paused) return 0;
    return 1;
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

job * find_job (pid_t pgid) {   // find active job using pgid.

  job * j;
  for (j = head_job; j; j = j->next)
    if (j->cpgid == pgid) return j;

  return NULL;
}

job * find_fg_job () {  // find newest job->paused/in_bg

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused || j->in_bg) return j;

    return NULL;
}

job * find_bg_job () {  // find newest job->paused

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused) return j;

    return NULL;
}

void print_job_info (job * j, const char * status) {
    fprintf(stdout, "[%d] -  %s\t%s\n", j->cpgid, status, j->line);
}

