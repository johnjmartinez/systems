#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LINE_MAX 200
#define	DELIM " \t\n\a\r"

bool tokenizer (char * line, char * _tokens[], int * count);
int parser (char * _tokens[]);
void send_cmd (char * line);

#endif /* CLIENT_H */

