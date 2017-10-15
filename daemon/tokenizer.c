#include "daemon.h"

bool tokenizer (char * line, char * _tokens[], int * count, int sckt) {

    if ( line[strlen(line)-1] != '\n' ) {           // check if line too long
        char * err = "ERROR: command too long -- 200 chars max\n";
        write (sckt, err, strlen(err));
        return true;
    }

    int i = 0;
    char * pointer = strtok(line, DELIM);           // strtok destroys string
    do {
       _tokens[i++] = pointer;
	} while ( (pointer = strtok(NULL, DELIM)) != NULL );

    _tokens[i] = NULL;                              // make last token/cmd_arg NULL
    * count = i;
    
    return false;
}

bool parser (char * _tokens[], int * pip, int * fwd, int * bck ) {    
    
    for( int i = 0; _tokens[i] != NULL; i++ )  {

        if ( strncmp(_tokens[i], "&", 1) == 0 ) {   // & !wait for completion
            if ( !i || _tokens[i+1] != NULL ) {
                printf("ERROR: \'&\' can only be used as last char, following a cmd\n");
                return true;
            }
            else if (*pip) {
                printf("ERROR: \'|\' and \'&\' not allowed in command line\n");
                return true;
            }
        }
        else if ( strncmp(_tokens[i], "<", 1) == 0 ) {  // BCK STDIN <
            if (*bck) {
                printf("ERROR: only one \'<\' allowed\n");
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                printf("ERROR: \'<\' can only be used as: \'(cmd) < (file)\'\n");
                return true;
            }
            *bck = i; 
        }
        else if ( strncmp(_tokens[i], ">", 1) == 0 ) {  // FWD STDOUT >
            if (*fwd) {
                printf("ERROR: only one \'>\' allowed\n");
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                printf("ERROR: \'>\' can only be used as: \'(cmd) > (file)\'\n");
                return true;
            }
            *fwd = i; 
        }
        else if ( strncmp(_tokens[i], "|", 1) == 0 ) {  // PIPE STDOUT | STDIN
            if (*pip) {
                printf("ERROR: only one \'|\' allowed\n");
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                printf("ERROR: \'|\' can only be used as: \'(cmd) | (cmd)\'\n");
                return true;
            }
            *pip = i;
        }
    }
    return false;
}

bool valid (int pip, int out, int in) {                 // CONSTRAINT CHECKING
    if ( (pip < in) && pip ) {
        printf("ERROR: invalid job: \'|\' followed by \'<\'\n");
        return false;
    }
    else if ( (out < pip) && out ) {
        printf("ERROR: invalid job: \'>\' followed by \'|\'\n");
        return false;
    }
    return true;
}
