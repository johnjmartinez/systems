#include "yash.h"

int cid1, cid2, status;

bool executor (char * cmds[], int pip, int out, int in, int count, char * line ) {

    fflush (0);
    if (pip) cmds[pip] = NULL;
    if (in)  cmds[in]  = NULL;
    if (out) cmds[out] = NULL;

    send_to_bg = (strncmp (cmds[count-1],"&",1) == 0);
    if (send_to_bg) cmds[count-1] = NULL;

    if ( (count==1) && (strncmp (cmds[0],"fg",2)==0) ) {         // FG wait for completion

        job * i;
        if ( (i = find_fg_job ()) == NULL ) return false;

        cgid = i->cpgid;
        i->in_bg = 0;
        kill(- i->cpgid, SIGCONT);
        waitpid(- i->cpgid, &status, WUNTRACED );
        i->status = status;
    }
    else if ( (count==1) && (strncmp (cmds[0],"bg",2)==0) ) {    // BG !wait for completion

        job * i;
        if ( (i = find_fg_job ()) == NULL ) return false;

        cgid = 0;
        i->in_bg = 1;
        kill(- i->cpgid, SIGCONT);
    }
    else if ( (count==1) && (strncmp (cmds[0],"jobs",4)==0) ) {  // JOBS
        job_list ();
    }
    else if ( (strncmp (cmds[0],"cd",2)==0) ) {                  // CD -- chdir ()
        if ( count==2 ) {                                        // TODO:
            if (chdir (cmds[1]) != 0)                            // update env $*WD
                perror ("yash");
        }
        else if ( count==1 ) {
            if (chdir(getenv ("HOME")) != 0)
                perror ("yash");
        }
    }

    else if ( !pip && !out && !in )                             // NO REDIRECTS
        exec_one ( cmds, new_job(line) );

    else if ( !pip && out && !in )                              // only out > REDIRECT
        exec_fwd ( cmds, cmds[out+1], new_job(line) );

    else if ( !pip && !out && in )                              // only in < REDIRECT
        exec_bck ( cmds, cmds[in+1], new_job(line) );

    else if ( pip && !out && !in )                              // only pipe | REDIRECT
        exec_pipe ( &cmds[0], &cmds[pip+1], new_job(line) );

    else if ( !pip && out && in )                               // both in/out, no pipe REDIRECT
        exec_in_out ( cmds, cmds[in+1], cmds[out+1], new_job(line) );

    else if ( pip && out && in )                                // both in/out w/ pipe REDIRECT
        exec_in_pipe_out ( &cmds[0], &cmds[pip+1], cmds[in+1], cmds[out+1], new_job(line) );

    else if ( pip && !out && in )                               // in w/ pipe REDIRECT
        exec_in_pipe ( &cmds[0], &cmds[pip+1], cmds[in+1], new_job(line) );

    else if ( pip && out && !in )                               // out w/ pipe REDIRECT
        exec_pipe_out ( &cmds[0], &cmds[pip+1], cmds[out+1], new_job(line) );

    return false;
}

void exec_one (char * cmd[], job * j) {

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");

    else if (!cid1) {   // CHILD
        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        log_job (cid1, j);
}

void exec_fwd (char * cmd[], char * f_out, job * j) {

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");

    else if (!cid1) {   // CHILD

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 )  perror ("ERROR: open failed");
        dup2 (fwd, 1);
        close (fwd);

        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        log_job (cid1, j);
}

void exec_bck (char * cmd[], char * f_in, job * j) {

    if ( (cid1=fork ()) < 0 )  perror ("ERROR: fork failed");

    else if (!cid1) {   // CHILD

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (bck = open (f_in, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (bck, 0);
        close (bck);

        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        log_job (cid1, j);
}

void exec_in_out (char * cmd[], char * f_in, char * f_out, job * j) {

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");

    else if (!cid1) {   // CHILD

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 ) perror ("ERROR: open failed");
        if ( (bck = open (f_in, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (fwd, 1); close (fwd);
        dup2 (bck, 0); close (bck);

        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        log_job (cid1, j);
}

void exec_pipe (char * cmd1[], char * cmd2[], job * j) {

    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");

    else if (!cid1) {   // CHILD_1

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);

        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT

        close (pipfd[1]);    // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");

        else if (!cid2) {   // CHILD_2

            setpgid(STDIN_FILENO, cid1);

            close (0);
            dup2 (pipfd[0], 0);

            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else {              // PARENT_
            close (pipfd[0]);
            log_job (cid1, j);
        }
    }
}

void exec_pipe_out (char * cmd1[], char * cmd2[], char * f_out, job * j) {

    //this is assuming a | b > c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");

    else if (!cid1) {   // CHILD_1

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);

        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT

        close (pipfd[1]);    // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");

        else if (!cid2) {   // CHILD_2

            setpgid(STDIN_FILENO, cid1);

            if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 )  perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);

            close (0);
            dup2 (pipfd[0], 0);

            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {             // PARENT_
            close (pipfd[0]);
            log_job (cid1, j);
        }
    }
}

void exec_in_pipe (char * cmd1[], char * cmd2[], char * f_in, job * j) {

    //this is assuming a < b | c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");

    else if (!cid1) {   // CHILD_1

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (bck = open (f_in, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (bck, 0); close (bck);

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);

        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT

        close (pipfd[1]);    // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");

        else if (!cid2) {   // CHILD_2

            setpgid(STDIN_FILENO, cid1);

            close (0);
            dup2 (pipfd[0], 0);

            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {             // PARENT_
            close (pipfd[0]);
            log_job (cid1, j);
        }
    }
}

void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * f_in, char * f_out, job * j) {

    //this is assuming a < b | c > d ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");

    else if (!cid1) {   // CHILD_1

        //setsid();
		cgid = getpid ();
		if (setpgid (cgid, cgid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (bck = open (f_in, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (bck, 0); close (bck);

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);

        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT

        close (pipfd[1]);    // done by both parent and child2
        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");

        else if (!cid2) {   // CHILD_2

            setpgid(STDIN_FILENO, cid1);

            if ( (fwd = open (f_out, O_FOUT, 0644)) < 0 ) perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);

            close (0);
            dup2 (pipfd[0], 0);

            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {             // PARENT_
            close (pipfd[0]);
            log_job (cid1, j);
        }
    }
}
