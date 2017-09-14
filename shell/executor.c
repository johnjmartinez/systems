#include "yash.h"

bool executor (char * _tokens[], int pip, int out, int in, int count ) {

    fflush (0);
    if (pip) _tokens[pip] = NULL;
    if (in)  _tokens[in]  = NULL;
    if (out) _tokens[out] = NULL;   
    
    bool send_to_bg = (strncmp (_tokens[count-1],"&",2) == 0);
    
    if ( (count==1) && (strncmp (_tokens[0],"fg",2)==0) ) {          // FG wait for completion
        printf ("\t - found fg\n");
    }
    else if ( (count==1) && (strncmp (_tokens[0],"bg",2)==0) ) {     // BG !wait for completion
        printf ("\t - found bg\n");
    }
    else if ( (count==1) && (strncmp (_tokens[0],"jobs",4)==0) ) {   // JOBS
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
    else if ( !pip && !out && !in ) {   // NO REDIRECTS
        exec_one ( _tokens);
    }
    else if ( !pip && out && !in ) {    // only fwd > REDIRECT
        exec_fwd ( _tokens, _tokens[out+1]);
    }
    else if ( !pip && !out && in ) {    // only bck < REDIRECT
        exec_bck ( _tokens, _tokens[in+1]);
    }
    else if ( pip && !out && !in ) {    // only pipe | REDIRECT
        exec_pipe ( &_tokens[0], &_tokens[pip+1]);
    }
    else if ( !pip && out && in ) {     // both in/out, no pipe REDIRECT
        exec_in_out ( _tokens, _tokens[in+1], _tokens[out+1]);
    }
    else if ( pip && out && in ) {      // both in/out w/ pipe REDIRECT
        exec_in_pipe_out ( &_tokens[0], &_tokens[pip+1], _tokens[in+1], _tokens[out+1] );
    }

    return false;
}

void exec_one (char * cmd[]) {

    pid_t cid;
    int status;

    if ( (cid=fork ()) < 0 )
        perror ("ERROR: fork failed");
    else if (!cid) {         // CHILD
        execvp (cmd[0], cmd);
        perror ("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_pipe (char * cmd1[], char * cmd2[]) {

    pid_t cid1, cid2;
    int status1, status2;
    int pipfd[2];
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 )
        perror ("ERROR: fork1 failed");
    else if (!cid1) {       // CHILD_1
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1");
    }
    else {                  // _PARENT
        waitpid (cid1, &status1, 0);
        close (pipfd[1]);

        if ( (cid2=fork ()) < 0 )
            perror ("ERROR: fork2 failed");
        else if (!cid2) {   // CHILD_2
            close (0);
            dup2 (pipfd[0], 0);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2");
        }
        else                // PARENT_
            waitpid (cid2, &status2, 0);
    }
}

void exec_fwd (char * cmd[], char * fileout) {
    pid_t cid;
    int status, fwd;

    if ( (cid=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid) {         // CHILD
        if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) 
                perror ("ERROR: open failed");
                
        dup2 (fwd, 1);
        close (fwd);
        execvp (cmd[0], cmd);
        perror ("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_bck (char * cmd[], char * filein) {
    pid_t cid;
    int status, bck;

    if ( (cid=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid) {         // CHILD
        if ( (bck = open (filein, O_FIN)) < 0 )
            perror ("ERROR: open failed");
            
        dup2 (bck, 0);
        close (bck);
        execvp (cmd[0], cmd);
        perror ("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_in_out (char * cmd[], char * filein, char * fileout) {
    pid_t cid;
    int status, fwd, bck;

    if ( (cid=fork ()) < 0 ) 
        perror ("ERROR: fork failed");
    else if (!cid) {         // CHILD
        if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) 
                perror ("ERROR: open failed");
        if ( (bck = open (filein, O_FIN)) < 0 )
            perror ("ERROR: open failed");
            
        dup2 (fwd, 1); close (fwd);
        dup2 (bck, 0); close (bck);
        
        execvp (cmd[0], cmd);
        perror ("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_in_pipe_out (char * cmd1[], char * cmd2[], char * filein, char * fileout) {
    
    //this is assuming a < b | c > d ---  which makes more sense than anything else
    pid_t cid1, cid2;
    int status1, status2, fwd, bck;
    int pipfd[2];
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {       // CHILD_1
    
        if ( (bck = open (filein, O_FIN)) < 0 )
            perror ("ERROR: open failed");    
        dup2 (bck, 0); close (bck);
    
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1");
    }
    else {                  // _PARENT
        waitpid (cid1, &status1, 0);
        close (pipfd[1]);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) 
                perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);
 
            close (0);
            dup2 (pipfd[0], 0);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2");
        }
        else                // PARENT_
            waitpid (cid2, &status2, 0);
    }
}

void exec_pipe_out (char * cmd1[], char * cmd2[], char * fileout) {

    //this is assuming a | b > c ---  which makes more sense than anything else
    pid_t cid1, cid2;
    int status1, status2, fwd;
    int pipfd[2];
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {       // CHILD_1

        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1");
    }
    else {                  // _PARENT
        waitpid (cid1, &status1, 0);
        close (pipfd[1]);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            if ( (fwd = open (fileout, O_FOUT, 0644)) < 0 ) 
                perror ("ERROR: open failed");
            dup2 (fwd, 1); close (fwd);
 
            close (0);
            dup2 (pipfd[0], 0);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2");
        }
        else                // PARENT_
            waitpid (cid2, &status2, 0);
    }
}


void exec_in_pipe (char * cmd1[], char * cmd2[], char * filein) {

    //this is assuming a < b | c ---  which makes more sense than anything else
    pid_t cid1, cid2;
    int status1, status2, bck;
    int pipfd[2];
    pipe (pipfd);

    if ( (cid1=fork ()) < 0 ) perror ("ERROR: fork1 failed");
    
    else if (!cid1) {       // CHILD_1
    
        if ( (bck = open (filein, O_FIN)) < 0 )
            perror ("ERROR: open failed");    
        dup2 (bck, 0); close (bck);
    
        close (pipfd[0]);
        close (1);
        dup2 (pipfd[1], 1);
        
        execvp (cmd1[0], cmd1);
        perror ("ERROR_1");
    }
    else {                  // _PARENT
        waitpid (cid1, &status1, 0);
        close (pipfd[1]);

        if ( (cid2=fork ()) < 0 ) perror ("ERROR: fork2 failed");
        
        else if (!cid2) {   // CHILD_2
        
            close (0);
            dup2 (pipfd[0], 0);
            execvp (cmd2[0], cmd2);
            perror ("ERROR_2");
        }
        else                // PARENT_
            waitpid (cid2, &status2, 0);
    }
}

