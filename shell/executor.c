#include "yash.h"

bool executor (char * _tokens[], int pip, int in, int out, int count ) {

    bool send_to_bg = false;
    if ( strncmp(_tokens[count-1],"&",2)==0 ) {
        send_to_bg = true; printf("\t - send to bg\n");
    }

    if ( (count==1) && (strncmp(_tokens[0],"fg",2)==0) ) {          // FG wait for completion
        printf("\t - found fg\n");
    }
    else if ( (count==1) && (strncmp(_tokens[0],"bg",2)==0) ) {     // BG !wait for completion
        printf("\t - found bg\n");
    }
    else if ( (count==1) && (strncmp(_tokens[0],"jobs",4)==0) ) {   // JOBS
        printf("\t - found \'jobs\'\n");
    }
    else if ( (strncmp(_tokens[0],"cd",2)==0) ) {                   // CD -- chdir()
        if ( count==2 ) {                                           // TODO:
            if (chdir(_tokens[1]) != 0)                             // update env $*WD
                perror("yash");
        }
        else if ( count==1 ) {
            if (chdir(getenv("HOME")) != 0)
                perror("yash");
        }
    }
    else if ( !pip && !out && !in ) {   // NO REDIRECTS
        exec_one( _tokens);
    }
    else if ( !pip && out && !in ) {    // only fwd > REDIRECT
        _tokens[out] = NULL;
        exec_fwd( _tokens, _tokens[out+1]);
    }
    else if ( !pip && !out && in ) {    // only bck < REDIRECT
        _tokens[in] = NULL;
        exec_bck( _tokens, _tokens[in+1]);
    }

    return false;
}

void exec_one (char * cmd[]) {

    pid_t cid;
    int status;

    if ( (cid=fork()) < 0 ) {
        perror ("ERROR: fork failed");
    }
    else if (!cid) {         // CHILD
        execvp(cmd[0], cmd);
        perror("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_pipe (char * cmd1[], char * cmd2[]) {

    pid_t cid1, cid2;
    int status1, status2;
    int pipfd[2];
    pipe(pipfd);

    if ( (cid1=fork()) < 0) {
        perror ("ERROR: fork1 failed");
    }
    else if (!cid1) {       // CHILD_1
        close(pipfd[0]);
        close(1);
        dup2(pipfd[1], 1);
        execvp(cmd1[0], cmd1);
        perror("ERROR_1");
    }
    else {                  // _PARENT
        waitpid (cid1, &status1, 0);
        close(pipfd[1]);

        if ( (cid2=fork()) < 0) {
            perror ("ERROR: fork2 failed");
        }
        else if (!cid2) {   // CHILD_2
            close(0);
            dup2(pipfd[0], 0);
            execvp(cmd2[0], cmd2);
            perror("ERROR_2");
        }
        else                // PARENT_
            waitpid (cid2, &status2, 0);
    }
}

void exec_fwd (char * cmd[], char * fileout) {
    pid_t cid;
    int status, fwd;

    if ( (cid=fork()) < 0 ) {
        perror ("ERROR: fork failed");
    }
    else if (!cid) {         // CHILD
        fwd = open(fileout, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        close(1);
        dup(fwd); // assigned out (1) by default
        execvp(cmd[0], cmd);
        perror("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}

void exec_bck (char * cmd[], char * filein) {
    pid_t cid;
    int status, bck;

    if ( (cid=fork()) < 0 ) {
        perror ("ERROR: fork failed");
    }
    else if (!cid) {         // CHILD
        bck = open(filein, O_RDONLY);
        close(0);
        dup(bck); // assigned in (0) by default
        execvp(cmd[0], cmd);
        perror("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);
}
