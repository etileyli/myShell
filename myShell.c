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

int readFile(char *fileName ){
	// Usage:
	// char *ptr = "temp4.txt";
	// if (!readFile(ptr))
	// 	printf("File exists!\n");
	// else
	// 	printf("File %s does not exist!\n", ptr);
	if( access(fileName, F_OK ) != -1 ) {
		FILE *fptr = fopen(fileName, "r");
		char c;
		while (c != EOF){
				printf ("%c", c);
				c = fgetc(fptr);
		}
		fclose(fptr);
		return 0;
	}
	else {
		printf("%s not found!\n", fileName);
	}
	return 1;
}

int launchSingleCommandProcess(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	int numOfArguments = inputLine->commands->info.com->num_of_args;
	char **argv = malloc((numOfArguments + 2) * sizeof(char *));	// command, arguments and NULL

	argv[0] = commandName;
	int i;
	for (i = 1; i < numOfArguments + 1; i++){
		argv[i] = commandArgs[i-1];
		printf("%s\n", argv[i]);
	}
	argv[i] = NULL;		// should end with NULL
	// char *argv[] = {"ls", "-l", NULL};

	pid_t childPid;
	// Fork
	if ((childPid = fork()) < 0)
		perror("error in fork()");
	else if (childPid == 0) {	// in child process
		// printf("in child\n");
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
		// printf("in parent\n");
	return 0;
}

int launchBuiltInCommands(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;

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
			launchSingleCommandProcess(inputLine);
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

int launchRedirectionCommandProcess (char *commandName, char **commandArgs, char *fileToInput, char *fileToOutput){

	if (fileToInput != NULL)	// Input redirection
		;

	return 0;
}

// fd = open(fileToInput, O_RDONLY, 0);
// dup2(fd, stdin);
// Read file's content and print it to stdin

int execute(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	// enum command_type commandType = inputLine->commands->type;
	// int isBackground = inputLine->background;
	char *fileToInput = inputLine->commands->input;
	char *fileToOutput = inputLine->commands->output;
	int builtInCommandNumber = sizeof(builtInCommands) / sizeof(char *);

	// print_input(inputLine);

	for (int i = 0; i < builtInCommandNumber; i++) {
		if (strcmp(commandName, builtInCommands[i]) == 0) {
			return launchBuiltInCommands(inputLine);
		}
	}

	launchSingleCommandProcess(inputLine);

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
