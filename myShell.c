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
char quitPrompt[] = "exit";	// command to quit

char *builtInCommands[] = {
	"exit",
	"cd",
	"help"
};

int launchProcess(char *commandName){
	char* argv[] = {commandName, NULL};
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
	else 		// parent process
		// if (isBackground)
		// 	printf("Child in background [%d]\n", childPid);
		// else
			wait(&childPid);
	return 0;
}

int launchBuiltInCommands(char *commandName, char **commandArgs){

	// Exit command
	if (!strcmp(commandName, quitPrompt)){
		printf("Quitting myShell...\n");
		return 0;
	}
	else if (!strcmp(commandName, "cd")){
		if (commandArgs == NULL)
			printf("Please run cd command with proper argument.\n");
		else{
			chdir(commandArgs[0]);
			launchProcess("pwd");
		}
		return 1;
	}
	else if (!strcmp(commandName, "help")){
		printf("Some HELP stuff!\n");
		return 1;
	}
	else {	// Defensive
		printf("Built-in command cannot be found!\n");
		return 1;
	}
}

int execute(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	enum command_type commandType = inputLine->commands->type;
	int isBackground = inputLine->background;
	char *fileToInput;
	char *fileToOutput;
	int fd;
	int builtInCommandNumber = sizeof(builtInCommands) / sizeof(char *);

	// print_input(inputLine);

	for (int i = 0; i < builtInCommandNumber; i++) {
		if (strcmp(commandName, builtInCommands[i]) == 0) {
			return launchBuiltInCommands(commandName, commandArgs);
		}
	}

	launchProcess(commandName);

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

	// if ((fileToOutput = inputLine->commands->output) != NULL){
	// 	printf("%s\n", fileToOutput);
	// }

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
