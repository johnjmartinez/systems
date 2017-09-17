#include "yash.h"

bool parser (char * _tokens[], int * pip, int * fwd, int * bck ) {

    for( int i = 0; _tokens[i] != NULL; i++ )  {

        if ( strncmp(_tokens[i], "&", 1) == 0 ) {           // & !wait for completion
            if ( !i || _tokens[i+1] != NULL ) {
                printf("ERROR: \'&\' can only be used as last char, following a cmd\n");
                return true;
            }
            else if (*pip) {
                printf("ERROR: \'|\' and \'&\' not allowed in command line\n");
                return true;
            }
            // /*DEBUG*/ printf("found &");
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
            *bck = i; // /*DEBUG*/ printf("found <");
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
            *fwd = i; // /*DEBUG*/ printf("found >");
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
            *pip = i; // /*DEBUG*/ printf("found |");
        }
        // /*DEBUG*/ printf("\t%i %s\n", i, _tokens[i]);
    }
    return false;
}

bool valid (int pip, int out, int in) {     // CONSTRAINT CHECKING
    if ( (pip < in) && pip ) {
        printf("ERROR: invalid job: \'|\' followed by \'<\'\n");
        return false;
    }
    /* ACTUALLY VALID
    else if ( (out < in) && out ) {
        printf("ERROR: invalid job: \'>\' followed by \'<\'\n");
        return false;
    }*/
    else if ( (out < pip) && out ) {
        printf("ERROR: invalid job: \'>\' followed by \'|\'\n");
        return false;
    }
    return true;
}
