#include "yash.h"

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

    _tokens[i] = NULL;  // line[strlen(line)-1] != '\0';
    * count = i; 
    return false;
}
