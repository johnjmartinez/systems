#include "daemon.h"

// The following is heavily influenced by
// https://www.gnu.org/software/libc/manual/html_node/Job-Control.html

job * new_job (char * line, job * head_job) {

        line[strlen(line)-1] = '\0';    // get rid of '\n'
        job * nj = (job *) malloc ( sizeof (job) );
        nj->line = strdup(line);
        nj->cpgid = 0;
        nj->done = 0;
        nj->paused = 0;
        nj->status = -1;
        nj->notify = 0;
        nj->in_bg = 0;
        nj->next = head_job;

        head_job = nj;                  // front of list (push)
        return nj;
}

void log_job (pid_t pgid, job * j, int send_to_bg) {
    int status = -1, rc;
    j->cpgid = pgid;
    
    // /*DEBUG*/ printf("\n\tLOGGING: pid %d - %s", j->cpgid, j->line);

    if (!send_to_bg ) {                 // foreground
        j->cpgid = pgid;
        rc =  waitpid (j->cpgid, &status, WUNTRACED ) ;
        if (rc > 0)
            j->status = status;
    }
    else  {                              // background
        j->in_bg = 1;
        rc = waitpid (j->cpgid, &status, WUNTRACED |  WNOHANG );
        if (rc > 0)
            j->status = status;
    }
    
}

void job_notify (t_stuff * t) {                 // recycle statuses before prompt

    job * j, * jprev, * jnext;
    job * head_job = t->head_job;
    
    update_status (head_job);                   // update status info for jobs
    
    jprev = NULL;
    for (j = head_job; j; j = jnext) {
        jnext = j->next;

        if (j->done) {                          // notify about & delete done job

            if (j->in_bg) {
                print_job_info (j, "Done   ", t->s_fd);
                j->notify = 1;
            }

            if (jprev)
                jprev->next = jnext;
            else
                head_job = jnext;

            //if (j) 
            //    free (j);
        }
        
        if (j->paused && !j->notify)  {
            print_job_info (j, "Stopped", t->s_fd);
            j->notify = 1;
        }
        
        jprev = j;
    }
    
    if ( t->write_ok && ((head_job == NULL) || (head_job->cpgid != t->cgid)) ) {
        if ( write (t->s_fd, "\n# ", 3) < 0) 
            error_n_exit("ERROR writing to socket");
        t->write_ok = false;
    }
}

void update_status (job * head_job) {

    int status = -1234;
    job * j;
    for (j = head_job; j; j = j->next) {

        int rc = waitpid (j->cpgid, &status, WUNTRACED | WCONTINUED | WNOHANG );

        // /*DEBUG*/ //usr/include/x86_64-linux-gnu/bits/waitflags.h
        // /*DEBUG*/ //usr/include/x86_64-linux-gnu/sys/wait.h
        // /*DEBUG*/ int  t=0, c=0, e=0, s=0;
        // /*DEBUG*/ if (WIFSTOPPED (status))   t = 1;
        // /*DEBUG*/ if (WIFCONTINUED (status)) c = 1;
        // /*DEBUG*/ if (WIFEXITED (status))    e = 1;
        // /*DEBUG*/ if (WIFSIGNALED (status))  s = 1;
        // /*DEBUG*/ if (rc>0 && status!=-1234)
        // /*DEBUG*/     printf ("\n\t%d\tUPDATE raw_status %d of %d\t[%d,%d,%d,%d]",
        // /*DEBUG*/               rc, status, j->cpgid, t, c, e, s );

        if (rc > 0) {
            j->status = status;
            if ( WIFCONTINUED (status) ) {
                j->paused = 0;
            }
            else if ( WIFEXITED (status) || WIFSIGNALED (status) )
                if(!j->paused) j->done = 1;
        }
        else if (rc < 0 )   // most likely dead
            j->done = 1;
    }
}

void job_list (t_stuff * t) {       // a.k.a. JOBS
    job * j;   
    update_status (t->head_job);    // update status info for jobs

    for (j = t->head_job; j; j = j->next) {
        if (j->in_bg) {
            print_job_info (j, get_status_str(j), t->s_fd);
            j->notify = 0;
        }
    }
}

void job_list_all (t_stuff * t) {  // DEBUG - list everything in job table
    job * j;
    char out[32];
    
    write (t->s_fd, "DEBUG\n", 6);
    for (j = t->head_job; j; j = j->next) {
        snprintf ( out, 32, "%d %d\t", j->status, j->in_bg);
        write (t->s_fd, out, strlen(out));
        print_job_info (j, get_status_str(j), t->s_fd);
    }
}

char * get_status_str (job * j) {

    if (j->paused) return "Stopped";
    if (!j->paused && !j->done) return "Running";
    if (j->done) return  "Done    ";
    return ("LostWTF");
}

int running_job (job * j) {             // 1 if job's running

    if (!j->done && !j->paused) return 0;
    return 1;
}

void kill_jobs (job * head_job) {

    job * target;
    job * curr = head_job;
    if (curr != NULL) {
        while (curr->next != NULL) {
            target = curr;
            curr = curr->next;
            kill(- target->cpgid, SIGINT);
            if (target) free (target);
        }
        kill(- curr->cpgid, SIGINT);
        free(curr);
    }
}

job * find_job (pid_t pgid, job * head_job) {       // find active job using pgid.

  job * j;
  for (j = head_job; j; j = j->next)
    if (j->cpgid == pgid) return j;

  return NULL;
}

job * find_fg_job (job * head_job) {                // find newest job->paused/in_bg

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused || j->in_bg) return j;

    return NULL;
}

job * find_bg_job (job * head_job) {                // find newest job->paused

    job * j;
    for (j = head_job; j; j = j->next)
      if (j->paused) return j;

    return NULL;
}

void print_job_info (job * j, const char * status, int sckt) {
    char out[512];
    snprintf(out, 512, "[%d] -  %s\t%s\n", j->cpgid, status, j->line);
    write(sckt, out, strlen(out));
}

