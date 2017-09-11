#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define LINE_MAX 200
#define	DELIM " \t"

void get_tokens(char * tokens[], char * line) { 
    
    char * pointer = strtok(line, DELIM);
    int count;
    
    do {
       tokens[count++] = pointer; 
	} while ( (pointer = strtok(NULL, DELIM)) != NULL );

    tokens[count] = NULL;
}   

static void sgnl_catcher(int sig_num) {
    printf("\n- Caught signal %d\n# ",sig_num);
    fflush(stdout);
}


int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];
    
    signal(SIGINT,  sgnl_catcher);   // catch ctrl+c
    signal(SIGTSTP, sgnl_catcher);  // catch ctrl+z

    while(1) {
        printf("# ");
        fflush(stdout);
        if ( fgets(line, LINE_MAX, stdin) == NULL ) { // catch ctrl+d (EOF)
            printf("\n");
            return(0);
        }    
        else { // line not null
            if ( line[strlen(line)-1] != '\n' ) {   // check if line too long
                printf("ERROR: command too long -- 200 chars max.\n");
                while (getchar() != '\n' && getchar() != '\0'); // flush stdin 
            }
            else {
                line[strlen(line)-1] = '\0';
                get_tokens(_tokens, line);       
            }
        }
        
        for( int i = 0; _tokens[i] != NULL; i++ )  printf ("%i %s\n", i, _tokens[i]);
    }
    return(1);
}

