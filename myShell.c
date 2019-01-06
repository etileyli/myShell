#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"

int MAXCHAR = 512;
char prompt[] = ">>> ";	// command line prompt
char quitPrompt[] = "quit";	// command to quit

int execute(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	enum command_type commandType = inputLine->commands->type;
	int isBackground = inputLine->background;
	char *fileToInput;
	char *fileToOutput;
	int fd;

	// print_input(inputLine);

	if ((fileToInput = inputLine->commands->input) != NULL){
		if( access( fileToInput, F_OK ) != -1 ) {
    	printf("file %s exists!\n", fileToInput);
			// fd = open(fileToInput, O_RDONLY, 0);
			// dup2(fd, stdin);
			// Read file's content and print it to stdin
			FILE *fptr = fopen(fileToInput, "r");
			char c;
			while (c != EOF){
					printf ("%c", c);
					c = fgetc(fptr);
			}
			fclose(fptr);
			return 1;
		}
		else {
			printf("file %s does NOT exist!\n", fileToInput);
		}
	}

	if ((fileToOutput = inputLine->commands->output) != NULL){
		printf("%s\n", fileToOutput);
	}

	if (!strcmp(commandName, quitPrompt)){
			printf("Quitting myShell...\n");
			return 0;
	}

	if (commandType == NORMAL){
		char* argv[] = {commandName, NULL};	// for pwd
		pid_t childPid;
		// Fork
		if ((childPid = fork()) < 0)
			perror("error in fork()");
		else if (childPid == 0) {	// in child process
			// execvp()
			if (execvp(argv[0], argv) < 0){
				printf("%s: Command not found\n", argv[0]);
				exit(0);
			}
		}
		else	// in parent process
			if (isBackground)
				printf("Child in background [%d]\n", childPid);
			else
				wait(&childPid);
			}
		else if (commandType == SUBSHELL){

		}
		else
			printf("Unknown command.\n");

		return 1;
	}

char *readLine(){
  char *line = NULL;
  ssize_t bufsize = 0;
	bufsize = getline(&line, &bufsize, stdin);
  return line;
}

void loop(void){
	char *line;
	input *inputLine;
	// char **args;
	int status;

	do{
		printf("%s", prompt);
		fflush(stdout);

		// read the command entered by the user
		if(!strcmp(line = readLine(), "\n")){
			status = 1;
			continue;
		}

		// parse the command
		inputLine = parse(line);

		// execute command
		status = execute(inputLine);

	}while(status);
}

int main(int argc, char** argv) {

	printf("Welcome to myShell!\n");
	loop();

	// Perform any shutdown cleanup
	return 0;
}
