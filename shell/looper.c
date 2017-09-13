#include "yash.h"

int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];
    
    int pipe_pos, fwd_pos, bck_pos, count;
    bool skip;
    
    signal(SIGINT,  sgnl_catcher);  // catch ctrl+c
    signal(SIGTSTP, sgnl_catcher);  // catch ctrl+z

    while(1) {
    
        printf("# ");
        fflush(stdout);
        skip = false;
        
        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF) on empty line
            printf("\n");
            return(0);
        }   
        
        count = 0;
        skip = tokenizer(line, _tokens, &count);
        if (skip || (_tokens[0]==NULL)) continue;
        
        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser(_tokens, &pipe_pos, &fwd_pos, &bck_pos);
        if (skip) continue;
        
        if(!valid(pipe_pos, fwd_pos, bck_pos)) continue;
        
        printf ("p:%i f:%i b:%i cnt:%i\n", pipe_pos, fwd_pos, bck_pos, count);
        skip = executor(_tokens, pipe_pos, fwd_pos, bck_pos, count);
        if (skip) continue;
        
    }
    printf("\n");
    return(1);
}
