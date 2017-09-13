#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

bool tokenizer (char * line, char * _tokens[], int * count) ;

bool parser (char * _tokens[], int * pip, int * fwd, int * bck );
bool valid (int pip, int out, int in);     

bool executor (char * _tokens[], int pip, int in, int out, int count );

void exec_one (char * _tokens[]);
void exec_pipe(char * cmd1[], char * cmd2[]);
void exec_fwd (char * cmd[], char * fileout);
void exec_bck (char * cmd[], char * filein);
