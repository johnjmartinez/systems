#include "daemon.h"

bool executor (char * cmds[], int pip, int out, int in, int count, char * line, t_stuff * data) {

    int rc, status, to_bg;                                      // in cmd line
    job * head = data->head_job;
    int sckt = data->s_fd;

    to_bg = (strncmp (cmds[count-1],"&",1) == 0);
    if (to_bg) 
        cmds[count-1] = NULL;

    if ( (count==1) && (strncmp (cmds[0],"fg",2)==0) ) {        // FG wait for completion

        job * i;
        if ( (i = find_fg_job (head)) == NULL ) return false;
        
        status = -1;
        data->cgid = i->cpgid;
        i->in_bg = 0;
        i->paused = 0;
        
        kill (i->cpgid, SIGCONT);
        rc = waitpid (i->cpgid, &status, WUNTRACED | WCONTINUED ); 
        if (rc > 0) 
            i->status = status;     
    }
    else if ( (count==1) && (strncmp (cmds[0],"bg",2)==0) ) {   // BG !wait for completion

        job * i;
        if ( (i = find_bg_job (head)) == NULL ) return false;
        if (pip) return false;                                  // not for pipes

        status = -1;
        data->cgid = 0;
        i->in_bg = 1;
        i->paused = 0;
        
        kill (i->cpgid, SIGCONT);
        rc = waitpid (i->cpgid, &status, WUNTRACED | WCONTINUED | WNOHANG); 
        if (rc > 0) 
            i->status = status;  
    }
    else if ( (strncmp (cmds[0],"jobs",4)==0) ) {               // JOBS
         if (count==1) 
             job_list (data);
         else 
             job_list_all (data);
    }    
    else if ( (strncmp (cmds[0],"cd",2)==0) ) {                 // CD -- chdir ()
        if ( count==2 ) {                                       // TODO -- update env $*WD
            if (chdir (cmds[1]) != 0)                          
                 write (sckt, "ERROR: cd", 9);
        }
        else if ( count==1 ) {
            if (chdir(getenv ("HOME")) != 0)
                write (sckt, "ERROR: cd", 9);
        }
    }
    else {                                                      // ACTUAL COMMAND 
        if (pip) cmds[pip] = NULL;
        if (in)  cmds[in]  = NULL;
        if (out) cmds[out] = NULL;
        
        job * j = new_job(line, head);                          // create new job, add to job_list
        data->head_job = j;

        if ( !pip && !out && !in )                              // NO REDIRECTS
            exec_one ( cmds, j, to_bg, data);

        else if ( !pip && out && !in )                          // only out > REDIRECT
            exec_fwd ( cmds, cmds[out+1], j, to_bg, data );

        else if ( !pip && !out && in )                          // only in < REDIRECT
            exec_bck ( cmds, cmds[in+1], j, to_bg, data );

        else if ( pip && !out && !in )                          // only pipe | REDIRECT
            exec_pipe ( &cmds[0], &cmds[pip+1], j, to_bg, data );

        else if ( !pip && out && in )                           // both in/out, no pipe REDIRECT
            exec_in_out ( cmds, cmds[in+1], cmds[out+1], j, to_bg, data );

        else if ( pip && out && in )                            // both in/out w/ pipe REDIRECT
            exec_in_pipe_out ( &cmds[0], &cmds[pip+1], cmds[in+1], cmds[out+1], j, to_bg, data );

        else if ( pip && !out && in )                           // in w/ pipe REDIRECT
            exec_in_pipe ( &cmds[0], &cmds[pip+1], cmds[in+1], j, to_bg, data );

        else if ( pip && out && !in )                           // out w/ pipe REDIRECT
            exec_pipe_out ( &cmds[0], &cmds[pip+1], cmds[out+1], j, to_bg, data);
    }
    return false;
}

void exec_one (char * cmd[], job * j, int bg, t_stuff * t) {
    int cid1; 
    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid1) {                                           // CHILD
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)  
			error_n_exit ("ERROR: setpgid failed");
        
        dup2(t->s_fd, STDOUT_FILENO);
        dup2(t->s_fd, STDERR_FILENO);
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else {                                                        // PARENT
        t->cgid = cid1;
        log_job (cid1, j, bg);
        if (bg)
            t->write_ok = true;
    }
}

void exec_fwd (char * cmd[], char * f_out, job * j, int bg, t_stuff * t) {
    int cid1; 
    int fwd;    // fwd = out = fd for >

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid1) {                                           // CHILD
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)
  			error_n_exit ("ERROR: setpgid failed");

        if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 )  
            perror ("ERROR: open failed");
        
        dup2 (fwd, 1);
        close (fwd);
        
        dup2(t->s_fd, STDERR_FILENO);
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else {                                                        // PARENT
        t->cgid = cid1;
        log_job (cid1, j, bg);
        if (bg)
            t->write_ok = true;
    }
}

void exec_bck (char * cmd[], char * f_in, job * j, int bg, t_stuff * t) {
    int cid1; 
    int bck;    // bck = in = fd for <

    if ( (cid1=fork ()) < 0 )  
        perror ("ERROR: fork failed");
    else if (!cid1) {                                           // CHILD
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)  			
            error_n_exit ("ERROR: setpgid failed");

        if ( (bck = open (f_in, O_FIN)) < 0 ) 
            perror ("ERROR: open failed");
        
        dup2 (bck, 0);
        close (bck);

        dup2(t->s_fd, STDOUT_FILENO);
        dup2(t->s_fd, STDERR_FILENO);
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else {                                                        // PARENT
        t->cgid = cid1;
        log_job (cid1, j, bg);
        if (bg)
            t->write_ok = true;
    }
}

void exec_in_out (char * cmd[], char * f_in, char * f_out, job * j, int bg, t_stuff * t) {
    int cid1; 
    int fwd, bck;   // fwd = out = fd for >; bck = in = fd for <

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid1) {                                           // CHILD
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)
  			error_n_exit ("ERROR: setpgid failed");

        if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 ) 
            perror ("ERROR: open failed");
        if ( (bck = open (f_in, O_FIN)) < 0 ) 
            perror ("ERROR: open failed");
        
        dup2 (fwd, 1); close (fwd);
        dup2 (bck, 0); close (bck);

        dup2(t->s_fd, STDERR_FILENO);
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else {                                                        // PARENT
        t->cgid = cid1;
        log_job (cid1, j, bg);
        if (bg)
            t->write_ok = true;
    }
}

void exec_pipe (char * cmd1[], char * cmd2[], job * j, int bg, t_stuff * t) {
    int cid1, cid2; 
    int pipfd[2];   // | in cmd line
    pipe (pipfd); 

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork1 failed");
    else if (!cid1) {                                           // CHILD_1
        
        t->cgid = j->cpgid = getpid ();
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {                                                      // _PARENT
        close (pipfd[1]);                                       // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) 
            perror ("ERROR: fork2 failed");
        else if (!cid2) {                                       // CHILD_2
            setpgid(STDIN_FILENO, cid1);
            
            close (0);
            dup2 (pipfd[0], 0);

            dup2(t->s_fd, STDOUT_FILENO);
            dup2(t->s_fd, STDERR_FILENO);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else {                                                  // PARENT_
            close (pipfd[0]);
            log_job (cid1, j, bg);
        }
    }
}

void exec_pipe_out (char * cmd1[], char * cmd2[], char * f_out, job * j, int bg, t_stuff * t) {
    int cid1, cid2; 
    int fwd;        // fwd = out = fd for >
    int pipfd[2];   // | in cmd line

    //this is assuming a | b > c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork1 failed");
    else if (!cid1) {                                           // CHILD_1
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)
 			error_n_exit ("ERROR: setpgid failed");

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {                                                      // _PARENT
        close (pipfd[1]);                                       // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) 
            perror ("ERROR: fork2 failed");
        else if (!cid2) {                                       // CHILD_2
            setpgid(STDIN_FILENO, cid1);

            if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 )  
                perror ("ERROR: open failed");
            
            dup2 (fwd, 1); close (fwd);
            close (0);
            dup2 (pipfd[0], 0);

            dup2(t->s_fd, STDERR_FILENO);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {                                                 // PARENT_
            close (pipfd[0]);
            log_job (cid1, j, bg);
        }
    }
}

void exec_in_pipe (char * cmd1[], char * cmd2[], char * f_in, job * j, int bg, t_stuff * t) {
    int cid1, cid2; 
    int bck;        // bck = in = fd for <
    int pipfd[2];   // | in cmd line

    //this is assuming a < b | c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork1 failed");
    else if (!cid1) {                                           // CHILD_1
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)
  			error_n_exit ("ERROR: setpgid failed");

        if ( (bck = open (f_in, O_FIN)) < 0 ) 
            perror ("ERROR: open failed");
        
        dup2 (bck, 0); close (bck);
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
    
        dup2(t->s_fd, STDOUT_FILENO);
        dup2(t->s_fd, STDERR_FILENO);
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {                                                      // _PARENT
        close (pipfd[1]);                                       // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) 
            perror ("ERROR: fork2 failed");
        else if (!cid2) {                                       // CHILD_2
            setpgid(STDIN_FILENO, cid1);

            close (0);
            dup2 (pipfd[0], 0);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {                                                 // PARENT_
            close (pipfd[0]);
            log_job (cid1, j, bg);
        }
    }
}

void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * f_in, char * f_out, job * j, 
                       int bg, t_stuff * t) {
    int cid1, cid2; 
    int fwd, bck;   // fwd = out = fd for >; bck = in = fd for <
    int pipfd[2];   // | in cmd line

    //this is assuming a < b | c > d ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) 
        perror ("ERROR: fork1 failed");
    else if (!cid1) {                                           // CHILD_1
        
		t->cgid = j->cpgid = getpid ();
		if (setpgid (t->cgid, t->cgid) < 0)
  			error_n_exit ("ERROR: setpgid failed");

        if ( (bck = open (f_in, O_FIN)) < 0 ) 
            perror ("ERROR: open failed");

        dup2 (bck, 0); close (bck);
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {                                                      // _PARENT

        close (pipfd[1]);                                       // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) 
            perror ("ERROR: fork2 failed");
        else if (!cid2) {                                       // CHILD_2
            setpgid(STDIN_FILENO, cid1);

            if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 ) 
                perror ("ERROR: open failed");
            
            dup2 (fwd, 1); close (fwd);
            close (0);
            dup2 (pipfd[0], 0);
        
            dup2(t->s_fd, STDERR_FILENO);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {                                                 // PARENT_
            close (pipfd[0]);
            log_job (cid1, j, bg);
        }
    }
}
