#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Safe asprintf macro
#define SAFE_ASPRINTF(write_to, ...) { \
    char *tmp_string_for_extend = write_to; \
    asprintf(&(write_to), __VA_ARGS__); \
    free(tmp_string_for_extend); \
}

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


int main(int argc, char *argv[]) {

    char line[LINE_MAX];
    char * _tokens[LINE_MAX/3];

    while(1) {
        printf("# ");
        fflush(stdout);
    
        if ( fgets(line, LINE_MAX, stdin) != NULL ) { // check line not null
            if ( line[strlen(line)-1] != '\n' ) {   // check if line too long
                printf("ERROR: command too long -- 200 chars max.\n");
                while (getchar() != '\n');          // flush stdin for next call
            }
            else {
                line[strlen(line)-1] = '\0';
                get_tokens(_tokens, line);       
            }
        }
        
        for( int i = 0; _tokens[i] != NULL; i++ )  printf ("%i %s\n", i, _tokens[i]);
    }
}

