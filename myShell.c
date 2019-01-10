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
enum redirectedFileType {IN, OUT} redirFileType;

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

char *readLine(){
  char *line = NULL;
  ssize_t bufsize = 0;
	bufsize = getline(&line, &bufsize, stdin);
  return line;
}

int launchSingleCommandProcess(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	int numOfArguments = inputLine->commands->info.com->num_of_args;
	char **argv = malloc((numOfArguments + 2) * sizeof(char *));	// command, arguments and NULL

	argv[0] = commandName;
	int i = 1;
	for (i; i < numOfArguments + 1; i++){
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
			if (chdir(commandArgs[0]) != 0)
				printf("Error in changing directory! Please enter a valid path.\n");
			else{
				char *str = "pwd\n";
				input *pwdCommand = parse(str);
				launchSingleCommandProcess(pwdCommand);
			}
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

int launchRedirectionCommandProcess (input *inputLine, int fileType){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	char *inputFile = inputLine->commands->input;
	char *outputFile = inputLine->commands->output;
	int status;
	char **argv = malloc((3) * sizeof(char *));	// command, fileName and NULL

	// print_input(inputLine);

	int i = 0;
	argv[i++] = commandName;
	if (fileType == IN){
		argv[i++] = inputFile;
	}
	else if (fileType == OUT){
		if (commandArgs != NULL){
			argv[i++] = commandArgs[0];
		}
	}
	argv[i] = NULL;		// should end with NULL

	pid_t childPid;
	// Fork
	if ((childPid = fork()) < 0)
		perror("error in fork()");
	else if (childPid == 0) {	// in child process
		// printf("in child process\n");
		int fd;
		if (fileType == IN){
			fd = open(argv[1], O_RDONLY, 0);
			dup2(fd, 0);
		}else if (fileType == OUT){
			if( access( outputFile, F_OK ) != -1 ) {
				fd = open(outputFile, O_WRONLY, 0);
			} else {
		  	FILE *temp;
				if ((temp = fopen(outputFile, "w")) == NULL)
					printf("file open error\n");
					exit(-1);
			}
			dup2(fd, 1);
			// close(fd);
		}
		if (execvp(argv[0], argv) < 0){		// cat fileName in child process
			printf("%s: Command not found\n", argv[0]);
			exit(-1);
		}
	}
	else {
		// parent process
		pid_t w = waitpid(childPid, &status, WUNTRACED | WCONTINUED);
		if (w == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
		if (WIFEXITED(status)) {
      // printf("exited, status=%d\n", WEXITSTATUS(status));
    }
		else
			printf("child process returned abnormally!\n");
		// printf("status: %d\n", status);
		//printf("in parent\n");
		return 1;
	}
}

int execute(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	// enum command_type commandType = inputLine->commands->type;
	// int isBackground = inputLine->background;
	char *fileToInput = inputLine->commands->input;
	char *fileToOutput = inputLine->commands->output;
	int builtInCommandNumber = sizeof(builtInCommands) / sizeof(char *);

	// print_input(inputLine);

	if (fileToInput != NULL){
		//printf("Input redirection\n");
		redirFileType = IN;
		return launchRedirectionCommandProcess(inputLine, redirFileType);
	}
	else if (fileToOutput != NULL){
		// printf("Output redirection!\n");
		redirFileType = OUT;
		return launchRedirectionCommandProcess(inputLine, redirFileType);
	}

	for (int i = 0; i < builtInCommandNumber; i++) {
		if (strcmp(commandName, builtInCommands[i]) == 0) {
			return launchBuiltInCommands(inputLine);
		}
	}

	launchSingleCommandProcess(inputLine);

	return 1;
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
