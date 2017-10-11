#include "yash.h"

bool tokenizer (char * line, char * _tokens[], int * count) {

    if ( line[strlen(line)-1] != '\n' ) {   // check if line too long
        printf("ERROR: command too long -- 200 chars max\n");
        while (getchar() != '\n');          // flush stdin
        return true;
    }

    int i = 0;
    char * pointer = strtok(line, DELIM);   // strtok destroys string
    do {
       _tokens[i++] = pointer;
	} while ( (pointer = strtok(NULL, DELIM)) != NULL );

    _tokens[i] = NULL;                      // make last token/cmd_arg NULL
    * count = i;
    
    return false;
}
