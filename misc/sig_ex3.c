/* file sig_ex3.c: This is a more complex example of signals
    Author: Ramesh Yerraballi
    
    Attempt to mimic:
    prompt>> top | grep firefox
    
    Parent creates a pipe and two child processes with the write end of the pipe serving as 
    stdout for top and read end serving as stdin for grep.
    
    1st child execs top & creates a new session with itself as member leader of pgrp in it. 
    pgid is same as child's pid (pid_ch1).
    
    2nd child execs grep & joins pgrp that first child created.
    
    Ctr-c : parent relays a SIGINT to both children using kill(-pid_ch1,SIGINT) 
    The two child processes receive the SIGINT and their default behavior is to terminate. 
    Once they do that, parent reaps their exit status (using wait), prints and exits.
    
    Ctrl-z :  parent relays a SIGTSTP to both children using kill(-pid_ch1,SIGTSTP)
    Parent's waitpid() call unblocks when child receives STOP signal. 
    Parent waits for 4 secs and resumes the the child that STOP'd. 
    This happens once for each of the two children.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int pipefd[2];
int status, pid_ch1, pid_ch2, pid;

static void sig_int(int signo) {
    printf("Sending signals to grp:%d\n",pid_ch1); // grp id is pid of 1st in pipeline
    kill(-pid_ch1,SIGINT);
}
static void sig_tstp(int signo) {
    printf("Sending SIGTSTP to grp:%d\n",pid_ch1); // grp id is pid of 1st in pipeline
    kill(-pid_ch1,SIGTSTP);
}

int main(void) {
    //char ch[1]={0};
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(-1);
    }
    
    pid_ch1 = fork();
    if (pid_ch1 > 0) {  // _Parent
        printf("Child1 pid = %d\n",pid_ch1);
        
        pid_ch2 = fork();
        if (pid_ch2 > 0) {  // Parent_
            printf("Child2 pid = %d\n",pid_ch2);

            if (signal(SIGINT, sig_int) == SIG_ERR) printf("signal(SIGINT) error");
            if (signal(SIGTSTP, sig_tstp) == SIG_ERR) printf("signal(SIGTSTP) error");

            close(pipefd[0]); // close the pipe in the parent
            close(pipefd[1]);
            int count = 0;

            while (count < 2) {
            
                if ( (pid = waitpid(-1, &status, WUNTRACED | WCONTINUED)) < 0 ) {
                // check if child has been stopped(WUNTRACED) or continued(WCONTINUED)
                // wait does not take options: waitpid(-1,&status,0) is same as wait(&status)
                //  waitpid w/o optns: waits only for terminated child processes
                //  waitpid w/ optns: able to specify responses to other changes in child's status
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }

                if (WIFEXITED(status)) {
                    printf("child %d exited, status=%d\n", pid, WEXITSTATUS(status)); 
                    count++;
                }
                else if (WIFSIGNALED(status)) {
                    printf("child %d killed by signal %d\n", pid, WTERMSIG(status)); 
                    count++;
                }
                else if (WIFSTOPPED(status)) {
                    printf("%d stopped by signal %d\n", pid,WSTOPSIG(status));
                    sleep(4); // sleep for 4 seconds before sending CONT
                    printf("Sending CONT to %d\n", pid);
                    kill(pid,SIGCONT);
                } 
                else if (WIFCONTINUED(status)) 
                    printf("Continuing %d\n",pid);
            }
            exit(1);
        }       // END if (pid_ch2 > 0)
        else {  // Child 2
            sleep(1);

            setpgid(0,pid_ch1); // child2 joins grp whose grp id is same as child1's pid
            close(pipefd[1]);   // close the write end

            dup2(pipefd[0],STDIN_FILENO);

            char *myargs[3];
            myargs[0] = strdup("grep");     // program: "grep" (word count)
            myargs[1] = strdup("firefox");  // argument: "firefox"
            myargs[2] = NULL;               // marks end of array
            execvp(myargs[0], myargs);
        }
    }       // END if (pid_ch1 > 0)
    else {  // Child 1
        setsid();   // child 1 creates new session, new grp, and becomes leader -
                    // grp id is same as his pid: pid_ch1
                    
        close(pipefd[0]); // close the read end
        dup2(pipefd[1],STDOUT_FILENO);
        
        char *myargs[2];
        myargs[0] = strdup("top");  // program: "top" (writes to stdout which is now pipe)
        myargs[1] = NULL;
        execvp(myargs[0], myargs); 
    }
}   //END MAIN
