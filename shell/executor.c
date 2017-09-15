#include "yash.h"
int status1, status2;

bool executor (char * _tokens[], int pip, int out, int in, int count, job * j) {

    fflush (0);
    if (pip) _tokens[pip] = NULL;
    if (in)  _tokens[in]  = NULL;
    if (out) _tokens[out] = NULL;   
    
    send_to_bg = (strncmp (_tokens[count-1],"&",1) == 0);
    if (send_to_bg) _tokens[count-1] = NULL;
    
    if ( (count==1) && (strncmp (_tokens[0],"fg",2)==0) ) {         // FG wait for completion
        printf ("\t - found fg\n");
    }
    else if ( (count==1) && (strncmp (_tokens[0],"bg",2)==0) ) {    // BG !wait for completion
        printf ("\t - found bg\n");
    }
    else if ( (count==1) && (strncmp (_tokens[0],"jobs",4)==0) ) {  // JOBS
        printf ("\t - found \'jobs\'\n");
    }
    else if ( (strncmp (_tokens[0],"cd",2)==0) ) {                  // CD -- chdir ()
        if ( count==2 ) {                                           // TODO:
            if (chdir (_tokens[1]) != 0)                            // update env $*WD
                perror ("yash");
        }
        else if ( count==1 ) {
            if (chdir(getenv ("HOME")) != 0)
                perror ("yash");
        }
    }
    
    else if ( !pip && !out && !in )     // NO REDIRECTS
        exec_one ( _tokens);
    
    else if ( !pip && out && !in )      // only out > REDIRECT
        exec_fwd ( _tokens, _tokens[out+1]);
    
    else if ( !pip && !out && in )      // only in < REDIRECT
        exec_bck ( _tokens, _tokens[in+1]);
    
    else if ( pip && !out && !in )      // only pipe | REDIRECT
        exec_pipe ( &_tokens[0], &_tokens[pip+1]);
    
    else if ( !pip && out && in )       // both in/out, no pipe REDIRECT
        exec_in_out ( _tokens, _tokens[in+1], _tokens[out+1]);
    
    else if ( pip && out && in )        // both in/out w/ pipe REDIRECT
        exec_in_pipe_out ( &_tokens[0], &_tokens[pip+1], _tokens[in+1], _tokens[out+1] );
    
    else if ( pip && !out && in )       // in w/ pipe REDIRECT
        exec_in_pipe ( &_tokens[0], &_tokens[pip+1], _tokens[in+1] );
    
    else if ( pip && out && !in )       // out w/ pipe REDIRECT
        exec_pipe_out ( &_tokens[0], &_tokens[pip+1], _tokens[out+1] );

    return false;
}

void exec_one (char * cmd[]) {

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");
    
    else if (!cid1) {   // CHILD
        //setsid();    
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        if (!send_to_bg) waitpid (cid1, &status1, 0);
}


void exec_fwd (char * cmd[], char * fileout) {
    
    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");
    
    else if (!cid1) {   // CHILD
        
        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}
    
        if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 )  perror ("ERROR: open failed");
        dup2 (fwd, 1);
        close (fwd);

        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        if (!send_to_bg) waitpid (cid1, &status1, 0);
}

void exec_bck (char * cmd[], char * filein) {
    
    if ( (cid1=fork ()) < 0 )  perror ("ERROR: fork failed");
    
    else if (!cid1) {   // CHILD
    
        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (bck = open (filein, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (bck, 0);
        close (bck);

        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        if (!send_to_bg) waitpid (cid1, &status1, 0);
}

void exec_in_out (char * cmd[], char * filein, char * fileout) {
    
    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork failed");
    
    else if (!cid1) {   // CHILD
        
        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}
                
        if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) perror ("ERROR: open failed");
        if ( (bck = open (filein, O_FIN)) < 0 ) perror ("ERROR: open failed");
        dup2 (fwd, 1); close (fwd);
        dup2 (bck, 0); close (bck);
        
        execvp (cmd[0], cmd);
        perror ("ERROR"); _exit(1);
    }
    else                // PARENT
        if (!send_to_bg) waitpid (cid1, &status1, 0);
}

void exec_pipe (char * cmd1[], char * cmd2[]) {

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
    
        close (pipfd[1]);
        waitpid (cid1, &status1, 0);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            sleep(0.001);
            setpgid(STDIN_FILENO, cid1);
        
            close (0);
            dup2 (pipfd[0], 0);
            
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else {              // PARENT_
            close (pipfd[0]);
            waitpid (cid2, &status2, 0);
        }
    }
}

void exec_pipe_out (char * cmd1[], char * cmd2[], char * fileout) {

    //this is assuming a | b > c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {   // CHILD_1
    
        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
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
        close (pipfd[1]);
        waitpid (cid1, &status1, 0);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            sleep(0.001);
            setpgid(STDIN_FILENO, cid1);
        
            if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 )  perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);
 
            close (0);
            dup2 (pipfd[0], 0);
            
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {              // PARENT_
            close (pipfd[0]);
            waitpid (cid2, &status2, 0);
        }
    }
}

void exec_in_pipe (char * cmd1[], char * cmd2[], char * filein) {

    //this is assuming a < b | c ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {   // CHILD_1

        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}
    
        if ( (bck = open (filein, O_FIN)) < 0 ) perror ("ERROR: open failed");    
        dup2 (bck, 0); close (bck);
    
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT
        close (pipfd[1]);
        waitpid (cid1, &status1, 0);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            sleep(0.001);
            setpgid(STDIN_FILENO, cid1);
        
            close (0);
            dup2 (pipfd[0], 0);
            
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {               // PARENT_
            close (pipfd[0]);
            waitpid (cid2, &status2, 0);
        }
    }
}

void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * filein, char * fileout) {
    
    //this is assuming a < b | c > d ---  which makes more sense than anything else
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {   // CHILD_1
    
        //setsid();
		gid = getpid ();
		if (setpgid (gid, gid) < 0)  {
			perror ("ERROR: setpgid failed");
			exit (1);
		}

        if ( (bck = open (filein, O_FIN)) < 0 ) perror ("ERROR: open failed");    
        dup2 (bck, 0); close (bck);
    
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1"); _exit(1);
    }
    else {              // _PARENT
        close (pipfd[1]);
        waitpid (cid1, &status1, 0);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            sleep(0.001);
            setpgid(STDIN_FILENO, cid1);
            
            if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);
 
            close (0);
            dup2 (pipfd[0], 0);
            
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2"); _exit(1);
        }
        else  {             // PARENT_
            close (pipfd[0]);
            waitpid (cid2, &status2, 0);
        }
    }
}
