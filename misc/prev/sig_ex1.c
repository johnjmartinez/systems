#include "../shell/yash.h"

/* This example shows a "signal action function" Send the child various signals and observe operation. */
   
void ChildHandler (int sig, siginfo_t *sip, void *notused) {
    int status;
    printf ("The process generating the signal is PID: %d\n", sip->si_pid);
    fflush (stdout);
    status = 0;
    
    /* WNOHANG flag means if no news, don't wait*/
    if (sip->si_pid == waitpid (sip->si_pid, &status, WNOHANG)) { 
        /* SIGCHLD doesn't necessarily mean death - quick check */
        if (WIFEXITED(status)|| WTERMSIG(status)) 
            printf ("Child is gone\n"); /* dead */
        else
            printf ("ALIVE\n"); /* alive */
    }
    else {
        printf ("NO NEWS\n");   /* no news */
    }
}

int main() {

    struct sigaction action;
    action.sa_sigaction = ChildHandler; /* Note use of sigaction, not handler */
    sigfillset (&action.sa_mask);
    action.sa_flags = SA_SIGINFO;       /* Note flag, otherwise NULL in function*/
    sigaction (SIGCHLD, &action, NULL);

    fork();
    while (1)   {
        printf ("PID: %d\n", getpid());
        sleep(2);
    }
}
