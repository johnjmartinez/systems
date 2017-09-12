/**
	@file vsh.c
	@author George Gibson
	@date Wednesday 29th July 2015
	@brief vsh Shell Version 1.0 BETA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define LINESIZE 1024
#define RL_BUFSIZE 1024
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"

char **split_line(char *line) {
    int bufsize = TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "vsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
		
    token = strtok(line, TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
	
        if (position >= bufsize) {
            bufsize += TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
		        free(tokens_backup);
                fprintf(stderr, "vsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
	
        token = strtok(NULL, TOK_DELIM);
    }
    tokens[position] = NULL;
    
	return tokens;
}

int main(int argc, char **argv) {
	int finish = 0;
	char *user = getenv("USER");
	char hostname[1024];
	char *pwd = getenv("PWD");
    char line[LINESIZE] = {0};
    char lineCopy[LINESIZE] = {0};
    char *command = NULL;
    char **args;
    
    // Print out a welcome message
    printf("Welcome to vsh\n");
	
	// Print out the message of the day
    system("/bin/cat /etc/motd");
    
    printf("\n");
	
	gethostname(hostname, 1024);
	
    while (!finish) {
		// Update 'pwd' variable
		pwd = getenv("PWD");
		// Print out prompt
		printf("[");
        printf(user);
        printf("@");
        printf(hostname);
        printf("] ");
        printf(pwd);
        printf(" $ ");
        fflush(stdout);
        
        if(NULL == fgets(line, sizeof line, stdin)) {
			// If NULL, leave vsh
            finish = 1;
            printf("\nLeaving vsh\n");
        }
        else {
			// If there is something there...
            printf("The command read was %s", line);
            printf("\n");
            char *newLine = strchr(line, '\n');
            
            if(newLine != NULL) {
                *newLine = '\0';
                strcpy(lineCopy, line);
            }
            
            command = strchr(line, ' ');
            
            if(command != NULL) {
                *command++ = '\0';
                printf("Command= _%s_\n\n", line);                
            }
            
            args = split_line(line);
            
            if(strcmp(line, "") == 0) {
				fprintf(stderr, "vsh: Expected command\n");
			}
            else if(strcmp(line, "cd") == 0) {
				printf("cd not available in BETA versions of vsh");
			}
            else if(strcmp(line,"programmer") == 0) {
                printf("The programmer of vsh is George Gibson\n");
            }
            else if(strcmp(line,"ver") == 0) {
                printf("The current version is 1.0\n");
            }
            else if(strcmp(line, "help") == 0) {
				printf("Help for vsh\n\nType a command to run it. The builtin commands are : cd, programmer, ver, help and exit.Their functions are as follows:\n\ncd: Change Directory. Changes the current working directory.\n\nprogrammer: Display the programmers of vsh.\n\nver: Display the version of vsh you are running.\n\nhelp: Launches this program.\n\nexit: Leave vsh Shell.\n\nThese are the only builtins in vsh, but note that if you define a command in /bin with one of these names, your program will not run in vsh unless you use it's explicit name (/bin/yourprogram) rather than \"yourprogram\". vsh 1.0 written and programmed by George Gibson.\n");
			}
            else if(strcmp(line,"exit") == 0) {
				finish = 1;
				printf("\nLeaving vsh...\n");
				getchar();
			}
            else {  // Run the command
                pid_t pid;
				int err;

				pid=fork();
				
				if(pid == -1) {
					perror("vsh");
				}
				else if(pid == 0) {
					execvp(args[0], args);
				    perror("vsh");
				}
				else {
					waitpid(pid, &err, 0);
				}
            }
        }
    }
    
    return 0;
}
