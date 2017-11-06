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

bool parser (char * _tokens[], int * pip, int * fwd, int * bck, int sckt ) {    
    char * err;
    
    if (_tokens[0] == NULL) 
        return true;
    
    for( int i = 0; _tokens[i] != NULL; i++ )  {

        if ( strncmp(_tokens[i], "&", 1) == 0 ) {   // & !wait for completion
            if ( !i || _tokens[i+1] != NULL ) {
                err = "ERROR: \'&\' can only be used as last char, following a cmd\n";
                write(sckt, err, strlen(err));
                return true;
            }
            else if (*pip) {
                err = "ERROR: \'|\' and \'&\' not allowed in command line\n";
                write(sckt, err, strlen(err));
                return true;
            }
        }
        else if ( strncmp(_tokens[i], "<", 1) == 0 ) {  // BCK STDIN <
            if (*bck) {
                err = "ERROR: only one \'<\' allowed\n";
                write(sckt, err, strlen(err));
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                err = "ERROR: \'<\' can only be used as: \'(cmd) < (file)\'\n";
                write(sckt, err, strlen(err));
                return true;
            }
            *bck = i; 
        }
        else if ( strncmp(_tokens[i], ">", 1) == 0 ) {  // FWD STDOUT >
            if (*fwd) {
                err = "ERROR: only one \'>\' allowed\n";
                write(sckt, err, strlen(err));
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                err = "ERROR: \'>\' can only be used as: \'(cmd) > (file)\'\n";
                write(sckt, err, strlen(err));
                return true;
            }
            *fwd = i; 
        }
        else if ( strncmp(_tokens[i], "|", 1) == 0 ) {  // PIPE STDOUT | STDIN
            if (*pip) {
                err = "ERROR: only one \'|\' allowed\n";
                write(sckt, err, strlen(err));
                return true;
            }
            else if ( !i || (_tokens[i+1] == NULL) ) {
                err = "ERROR: \'|\' can only be used as: \'(cmd) | (cmd)\'\n";
                write(sckt, err, strlen(err));
                return true;
            }
            *pip = i;
        }
    }
    return false;
}

bool valid (int pip, int out, int in, int sckt) {       // CONSTRAINT CHECKING
    char * err;
    
    if ( (pip < in) && pip ) {
        err = "ERROR: invalid job: \'|\' followed by \'<\'\n";
        write(sckt, err, strlen(err));
        return false;
    }
    else if ( (out < pip) && out ) {
        err = "ERROR: invalid job: \'>\' followed by \'|\'\n";
        write(sckt, err, strlen(err));
        return false;
    }
    return true;
}
