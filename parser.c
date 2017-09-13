#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

static void sgnl_catcher(int sig_num) {
    printf("\n- Caught signal %d\n# ", sig_num);
    fflush(stdout);
}

bool tokenizer (char * line, char * _tokens[], int * count) {
    
    if ( line[strlen(line)-1] != '\n' ) {   // check if line too long
        printf("ERROR: command too long -- 200 chars max\n");
        while (getchar() != '\n'); // flush stdin 
        return true;
    }
    
    int i = 0;
    char * pointer = strtok(line, DELIM);  
      
    do {
       _tokens[i++] = pointer; 
	} while ( (pointer = strtok(NULL, DELIM)) != NULL );

    _tokens[i] = NULL;
    *count = i; 
    return false;
}

bool parser (char * _tokens[], int * pip, int * fwd, int * bck ) {
    
    for( int i = 0; _tokens[i] != NULL; i++ )  {        

        if ( strncmp(_tokens[i], "&", 1) == 0 ) {           // & !(wait for completion)
            if ( !i || _tokens[i+1] != NULL ) { 
                printf("ERROR: \'&\' can only be used as last char, following a cmd\n");
                return true;
            }
            //printf("found &");
        } 
        else if ( strncmp(_tokens[i], "<", 1) == 0 ) {      // BCK STDIN <
            if (*bck) {
                printf("ERROR: only one \'<\' allowed\n");
                return true;                    
            }                 
            else if ( !i || (_tokens[i+1] == NULL) ) { 
                printf("ERROR: \'<\' can only be used as: \'(cmd) < (file)\'\n");
                return true;
            }                
            *bck = i; // printf("found <");
        }      
        else if ( strncmp(_tokens[i], ">", 1) == 0 ) {      // FWD STDOUT > 
            if (*fwd) {
                printf("ERROR: only one \'>\' allowed\n");
                return true;                    
            }                 
            else if ( !i || (_tokens[i+1] == NULL) ) { 
                printf("ERROR: \'>\' can only be used as: \'(cmd) > (file)\'\n");
                return true;
            }                
            *fwd = i; // printf("found >");
        }      
        else if ( strncmp(_tokens[i], "|", 1) == 0 ) {      // PIPE STDOUT | STDIN 
            if (*pip) {
                printf("ERROR: only one \'|\' allowed\n");
                return true;                    
            }
            else if ( !i || (_tokens[i+1] == NULL) ) { 
                printf("ERROR: \'|\' can only be used as: \'(cmd) | (cmd)\'\n");
                return true;
            }
            *pip = i; // printf("found |");
        }      
        printf ("\t%i %s\n", i, _tokens[i]);
    }
    return false;
}

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
        
        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF)
            printf("\n");
            return(0);
        }    
 
        count = 0;
        skip = tokenizer(line, _tokens, &count);
        if (skip) continue;
        
        pipe_pos = 0; fwd_pos = 0; bck_pos = 0;
        skip = parser(_tokens, &pipe_pos, &fwd_pos, &bck_pos);
        printf ("p:%i f:%i b:%i\n", pipe_pos, fwd_pos, bck_pos);
        if (skip) continue;

    }
    printf("\n");
    return(1);
}

        /* MOVE TO EXEC part to do with jobs, cd
        else if ( strncmp(_tokens[i], "fg", 2) == 0 )  {    // FG wait for completion
            if ( !i && (_tokens[i+1] == NULL) ) 
                printf("found fg");
            else
                continue;            
        }
        else if ( strncmp(_tokens[i], "bg", 2) == 0  ) {    // BG !wait for completion
            if ( !i && (_tokens[i+1] == NULL) ) 
                printf("found bg");
            else
                continue;
        }*/     
