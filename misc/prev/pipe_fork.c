#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {

    int pipefd[2];
    pid_t cpid1, cpid2;
    char buf;
    
    pipe(pipefd);
    cpid1 = fork();

    if (cpid1 == 0) {       /* Child 1 writes argv[1] to pipe */
        char out[200];
        snprintf(out, 200, "%d : %s", getpid(), argv[1]);
        
        close(pipefd[0]);           /* Close unused read end */
        write(pipefd[1], out, strlen(out));        
        close(pipefd[1]);           /* Reader will see EOF */
        _exit(EXIT_SUCCESS);
    } 
    else {                  /* Parent */
        cpid2 = fork();
        
        if (cpid2 == 0) {   /* Child 2 reads from pipe */
            close(pipefd[1]);       /* Close unused write end */

            while (read(pipefd[0], &buf, 1) > 0)
                write(STDOUT_FILENO, &buf, 1);

            write(STDOUT_FILENO, " %\n", 3);
            close(pipefd[0]);
            _exit(EXIT_SUCCESS);           
        }
        else {
            wait(NULL);             /* Wait for child */        
            exit(EXIT_SUCCESS);
        }
    }
}
