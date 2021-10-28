// Program: CS344, smallsh
// Due date: 11/1/21
// Author: Megan Janitor

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAXINPUTLENGTH = 2048

void getCommand(char*[], int*, char[], char[], int);
void printExitStatus(int);
void execCmd(char*[], int*, struct sigaction, int*, char[], char[]);
void catchSIGTSTP(int);

// Main function takes command input, and executes the command. Handles blank lines or comments by re-prompting user for a command
int main(){
    // will store some vars first
    // Takes user args
    char* userInput[512];
    // Optional var for storing if user wants cmd to run in background
    int background = 0;
    // Optional inputfile argument
    char inputFile[256] = "";
    // Optional outputfile argument
	char outputFile[256] = "";
    // Gets the current process ID
    int pid = getpid();
    // Variable to store if shell should be exited
    int endProgram = 0;
    // var to store exit status
    int exitStatus = 0;

    // Signal Handlers
	
	// Ignore ^C
	struct sigaction sa_sigint = {0};
	sa_sigint.sa_handler = SIG_IGN;
	sigfillset(&sa_sigint.sa_mask);
	sa_sigint.sa_flags = 0;
	sigaction(SIGINT, &sa_sigint, NULL);

	// Redirect ^Z to catchSIGTSTP()
	struct sigaction sa_sigtstp = {0};
	sa_sigtstp.sa_handler = catchSIGTSTP;
	sigfillset(&sa_sigtstp.sa_mask);
	sa_sigtstp.sa_flags = 0;
	sigaction(SIGTSTP, &sa_sigtstp, NULL);

    do {
        // First we need to get the user's input
        getCommand(userInput, &background, inputFile, outputFile, pid);

        // continues if user input is a comment
        if (userInput[0][0] == '#') {
            continue;
        }
        // continues if user input is a blank line
        if (userInput[0][0] == '\0'){
            continue;
        }

        // BUILT-IN COMMANDS
        // if the user's input is equal to exit, we change the endProgram variable
        else if (strcmp(userInput[0], "exit") == 0){
            endProgram = 1;
        }
        // if the user's input is equal to cd we change directory
        else if (strcmp(userInput[0], "cd") == 0){
            // if the user specifies a directory to change to
            if (userInput[1]){
                // we try to change to the directory specified
                if (chdir(userInput[1]) == -1){
                    printf("Directory not found!");
                    //fflush(stdout);
                }

            }
            else {
                // if no directory argument provided, change to HOME env
                chdir(getenv("HOME"));
            }
        }
        // if the user's input is equal to status we find the status of the shell processes
        else if (strcmp(userInput[0], "status") == 0){
            printExitStatus(exitStatus);
        }
        // OTHER COMMANDS
        // If the user enters a command that isn't one of the built-in commands
        else{
            execCmd(userInput, &exitStatus, sa_sigint, &background, inputFile, outputFile);
        }
        // Reset variables
		for (i=0; input[i]; i++) {
			input[i] = NULL;
		}
		background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
    }
    while (endProgram != 0);

    return 0;

}

