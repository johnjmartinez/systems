#include "shell/yash.h"

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
            // DEBUG printf("found &");
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
            *bck = i; // DEBUG printf("found <");
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
            *fwd = i; // DEBUG printf("found >");
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
            *pip = i; // DEBUG printf("found |");
        } 
        // DEBUG printf("\t%i %s\n", i, _tokens[i]);
    }
    return false;
}

bool valid (int pip, int out, int in) {     // CONSTRAINT CHECKING
    if ( (pip < in) && pip ) {
        printf("ERROR: invalid job: \'|\' followed by \'<\'\n");
        return false;
    }
    else if ( (out < in) && out ) {
        printf("ERROR: invalid job: \'>\' followed by \'<\'\n");
        return false;
    }
    else if ( (out < pip) && out ) {
        printf("ERROR: invalid job: \'>\' followed by \'|\'\n");
        return false;
    }
    return true;
}

bool executor (char * _tokens[], int pip, int in, int out, int count ) {
        
        pid_t pid;
        int err;
        bool send_to_bg = false;
        
        if ( strncmp(_tokens[count-1],"&",2)==0 ) {
            send_to_bg = true; printf("\t - send to bg\n");
        }

        if ( (count==1) && (strncmp(_tokens[0],"fg",2)==0) ) {      // FG wait for completion
            printf("\t - found fg\n");
        }
        else if ( (count==1) && (strncmp(_tokens[0],"bg",2)==0) ) { // BG !wait for completion
            printf("\t - found bg\n");
        }   
        else if ( (count==1) && (strncmp(_tokens[0],"cd",2)==0) ) { // CD -- chdir()
            printf("\t - found cd\n");
        }
        else if ( (count==1) && (strncmp(_tokens[0],"jobs",4)==0) ) { // JOBS
            printf("\t - found \'jobs\'\n");
        }
        else if ( !pip && !out && !in ) {
            if ((pid=fork()) < 0) {
                perror ("ERROR: fork failed");
                return true;
            }
                
            else if (!pid) {         // CHILD
                execvp(_tokens[0], _tokens); 
                perror("ERROR");
            }
            else if (pid)            // PARENT
                waitpid (pid, &err, 0);       
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
  
