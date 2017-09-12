#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    
    int in, out;
    size_t got;
    char buffer[1024];
    
    in = open (argv[1], O_RDONLY);
    out = open (argv[2], O_TRUNC | O_CREAT | O_WRONLY, 0666);
    
    if ((in <= 0) || (out <= 0)) {
        fprintf (stderr, "Couldn't open a file\n");
        exit (errno);
    }
    
    dup2 (in, 0);
    dup2 (out, 1);
    close (in);
    close (out);
    
    while (1) {
        got = fread (buffer, 1, 1024, stdin);
        if (got <=0) break;
        fwrite (buffer, got, 1, stdout);
    }
}
