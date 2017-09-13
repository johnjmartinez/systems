#include "yash.h"

bool executor (char * _tokens[], int pip, int in, int out, int count ) {
        

    bool send_to_bg = false;
    if ( strncmp(_tokens[count-1],"&",2)==0 ) {
        send_to_bg = true; printf("\t - send to bg\n");
    }

    if ( (count==1) && (strncmp(_tokens[0],"fg",2)==0) ) {      // FG wait for completion
        printf("\t - found fg\n");
    }
    else if ( (count==1) && (strncmp(_tokens[0],"bg",2)==0) ) { // BG !wait for completion
        printf("\t - found bg\n");
    }   
    else if ( (count==1) && (strncmp(_tokens[0],"jobs",4)==0) ) { // JOBS
        printf("\t - found \'jobs\'\n");
    }
    else if ( (count==2) && (strncmp(_tokens[0],"cd",2)==0) ) { // CD -- chdir()
        printf("\t - found cd\n");
    }
    else if ( !pip && !out && !in ) {
        exec_1( _tokens);
    }
    return false;
}

void exec_1 (char * _tokens[]) {
    pid_t cid;
    int status;

    if ( (cid=fork()) < 0 ) {
        perror ("ERROR: fork failed");
    }
    else if (!cid) {         // CHILD
        execvp(_tokens[0], _tokens); 
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
        dup(fwd);
        execvp(cmd[0], cmd); 
        perror("ERROR");
    }
    else                    // PARENT
        waitpid (cid, &status, 0);       
}
