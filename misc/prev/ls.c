#include "../shell/yash.h"

int main(int argc, char *argv[]) {

    //int status;
    int pid;
    char *prog_argv[4];

    prog_argv[0]="ls";
    prog_argv[1]="-l";
    prog_argv[2]="/";
    prog_argv[3]=NULL;

    if ((pid=fork()) < 0) {
        perror ("Fork failed");
        exit(-1);
    }
    
    if (!pid)   //CHILD
        execvp (prog_argv[0], prog_argv);
    if (pid)    //PARENT
        waitpid (pid, NULL, 0);
}
