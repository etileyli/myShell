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
enum commandProcessType {SINGLE, REDIR} commProcType;

char *builtInCommands[] = {
	"exit",
	"cd",
	"help"
};

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
		// printf("%s\n", argv[i]);
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

int launchRedirectionCommandProcess(input *inputLine, int fileType){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	int commandArgNum = inputLine->commands->info.com->num_of_args;
	char *inputFile = inputLine->commands->input;
	char *outputFile = inputLine->commands->output;
	int status;
	char **argv = malloc((3 + commandArgNum) * sizeof(char *));	// command, arguments, fileName and NULL

	// print_input(inputLine);

	int i = 0;
	argv[i++] = commandName;
	if (fileType == IN){
		argv[i++] = inputFile;
	}
	else if (fileType == OUT){
		if (commandArgs != NULL){
			int j;
			for (j = 0; j < commandArgNum; j++ )
				argv[i++] = commandArgs[j];
		}
	}
	argv[i] = NULL;		// should end with NULL

	// for (int j = 0; j < commandArgNum + 2; j++ )
	// 	printf("%s\n", argv[j]);

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
			if(access(outputFile, F_OK ) != -1) {
				fd = open(outputFile, O_WRONLY, 0);
				dup2(fd, 1);
			} else {
		  	FILE *temp = fopen(outputFile, "w");
				if (temp == NULL){
					printf("file open error\n");
					exit(-1);
				}else{
					fd = open(outputFile, O_WRONLY, 0);
					dup2(fd, 1);
				}
			}
		}
		close(fd);
		if (execvp(argv[0], argv) < 0){
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
		// printf("in parent\n");
		return 1;
	}
}

int launchSemiColonCommandProcess(input *inputLine){
	int numOfCommands = inputLine->num_of_commands;
	char **argvList[numOfCommands];
	int commProcType[numOfCommands];
	int redirFileType[numOfCommands];
	// print_input(inputLine);

	int k;
	for (k = 0; k < numOfCommands; k++){
		// create argv for each command
		int i = 0;
		char **commandArgs = inputLine->commands[k].info.com->arguments;
		int numOfArguments = inputLine->commands[k].info.com->num_of_args;
		char **argv = malloc((numOfArguments + 2) * sizeof(char *));
		argv[i++] = inputLine->commands[k].info.com->name;
		char *inputFile = inputLine->commands[k].input;
		char *outputFile = inputLine->commands[k].output;


		if (inputFile != NULL){	// input redirected
			argv[i++] = inputFile;
			commProcType[k] = REDIR;
			redirFileType[k] = IN;
		}
		else if (outputFile != NULL){	// output redirected
			if (commandArgs != NULL){
				argv[i++] = commandArgs[0];
				commProcType[k] = REDIR;
				redirFileType[k] = OUT;
			}
		}
		else {	// single command
			if (commandArgs != NULL){
				int j;
				for (j = 0; j < numOfArguments; j++)
					argv[i++] = commandArgs[j];
			}
			commProcType[k] = SINGLE;
		}
		argv[i] = NULL;
		argvList[k] = argv;
	}

	// semi-colon
	for (int k = 0; k < 3; k++){
		if (commProcType[k] == SINGLE){
			pid_t childPid;
			// Fork
			if ((childPid = fork()) < 0)
				perror("error in fork()");
			else if (childPid == 0) {	// in child process
				// printf("in child\n");
				// printf("argvList[%d][0]: %s\n",k , argvList[k][0]);
				if (execvp(argvList[k][0], argvList[k]) < 0){
					printf("%s: Command not found\n", argvList[k][0]);
					exit(0);
				}
			}
			else 		// parent process
				wait(&childPid);
				// printf("in parent\n");
		}
		else if (commProcType[k] == REDIR){

			// concatenate commands and arguments into string. call redir function
			if (redirFileType[k] == IN){
				char temp[MAXCHAR];
				strcpy(temp, "cat < ");
				strcat(temp, argvList[k][1]);	// file name of input redirected command
				strcat(temp, "\n");
				input *cmd = parse(temp);
				// printf("input command: %s\n", temp);
				launchRedirectionCommandProcess(cmd, IN);
			}
			else if (redirFileType[k] == OUT){
				char temp[MAXCHAR];
				strcpy(temp, argvList[k][0]);	// copy command
				strcat(temp, " ");
				strcat(temp, "> ");
				int j;
				for (j = 0; j < inputLine->commands[k].info.com->num_of_args; j++){
					strcat(temp, argvList[k][j + 1]);
					strcat(temp, " ");
				}
				strcat(temp, "\n");
				input *cmd = parse(temp);
				// printf("output command: %s", temp);
				launchRedirectionCommandProcess(cmd, OUT);
			}
		}
	}
	// semi-colon

	return 1;
}

int launchPipedCommandProcess(input *inputLine){
	int numOfCommands = inputLine->num_of_commands;
	char *inputFile = inputLine->commands->input;
	char *outputFile = inputLine->commands->output;
	char **argvList[numOfCommands];

	print_input(inputLine);

	int k;
	for (k = 0; k < numOfCommands; k++){
		// create argv for each command
		int i = 0;
		char **commandArgs = inputLine->commands[k].info.com->arguments;
		int numOfArguments = inputLine->commands[k].info.com->num_of_args;
		char **argv = malloc((numOfArguments + 2) * sizeof(char *));
		argv[i++] = inputLine->commands[k].info.com->name;

		if (inputFile != NULL){	// input redirected
			argv[i++] = inputFile;
		}
		else if (outputFile != NULL){	// output redirected
			if (commandArgs != NULL){
				argv[i++] = commandArgs[0];
			}
		}
		else {	// single command
			if (commandArgs != NULL){
				int j;
				for (j = 0; j < numOfArguments; j++)
					argv[i++] = commandArgs[j];
			}
		}
		argv[i] = NULL;
		argvList[k] = argv;
	}

	// TEST
	for (int k = 0; k < 3; k++){
		pid_t childPid;
		// Fork
		if ((childPid = fork()) < 0)
			perror("error in fork()");
		else if (childPid == 0) {	// in child process
			printf("in child\n");
			printf("argvList[%d][0]: %s\n",k , argvList[k][0]);
			if (execvp(argvList[k][0], argvList[k]) < 0){
				printf("%s: Command not found\n", argvList[k][0]);
				exit(0);
			}
		}
		else 		// parent process
			wait(&childPid);
			printf("in parent\n");
	}
	// TEST

/*
	int fd[2];	// to hold fds of both ends of pipe
	int status1, status2;	// status' of child processes
	pid_t childPid1, childPid2;

	if (pipe(fd) < 0)
		perror("pipe error!\n");

	if (childPid1 = fork())	{
		if (childPid2 = fork()){	// parent process
			close(fd[0]);
			close(fd[1]);
			pid_t w1 = waitpid(childPid1, &status1, WUNTRACED | WCONTINUED);
			pid_t w2 = waitpid(childPid2, &status2, WUNTRACED | WCONTINUED);
			if ( (w1 == -1) || (w2 == -1)){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		}
		else if (childPid2 == 0){	// child process (pipe reader child)
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);
			// if (execvp(argv[0], argv) < 0){
			// 		printf("%s: Command not found\n", argv[0]);
			// 		exit(-1);
			// }
		}
	}
	else if (childPid1 == 0){//  child process (pipe writer child)
		close(fd[0]);
		dup2(fd[1], 1);
		close(fd[1]);
		// if (execvp(argv[0], argv) < 0){
		// 		printf("%s: Command not found\n", argv[0]);
		// 		exit(-1);
		// }
	}
*/
	return 1;
}

int execute(input *inputLine){
	char *commandName = inputLine->commands->info.com->name;
	char **commandArgs = inputLine->commands->info.com->arguments;
	// enum command_type commandType = inputLine->commands->type;
	// int isBackground = inputLine->background;
	char *fileToInput = inputLine->commands->input;
	char *fileToOutput = inputLine->commands->output;
	char del = inputLine->del;
	int builtInCommandNumber = sizeof(builtInCommands) / sizeof(char *);

	// print_input(inputLine);

	if (del == '|'){
		printf("It is a Pipe!\n");
		return launchPipedCommandProcess(inputLine);
	}
	else if (del == ';'){
		return launchSemiColonCommandProcess(inputLine);
	}

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
