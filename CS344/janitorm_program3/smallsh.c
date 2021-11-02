// Program: smallsh
// Class: CS344
// Author: Megan Janitor
// Due date: 11/1/21

// removes precompiler warnings
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_CMD_LENGTH	2048
#define MAX_ARGS	512

// Function prototypes
void catchSIGINT(int signo);
void catchSIGTSTP();
void isProcessRunning();
void getInput();
void removeEndChar(char *removeItem);
void runInBackground();
void expand();
void expandFromChild();
void doBuiltIns();
void forkProcess();
void executeCommand();

// Keeps the program running unless changed
int keepRunning = 1;
int status;
// Array for processes
int processes[20];
int numProcesses;
// Array for user input
char input[MAX_CMD_LENGTH];
// Flag for if process runs in background
bool backgroundFlag = false;
bool sigtstpFlag = false;
// will be 0 if no sigtstp flags have been raised
int sigNum = 0;
pid_t initPid = -1;
int childExitMethod = -1;


void main()
{
	// initialize struct for SIGINT later
	struct sigaction SIGINT_action = { 0 };
	// initialize struct fot SIGTSTP later
	struct sigaction SIGTSTP_action = { 0 };

	// Creates handler for SIGINT
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Creates handler for SIGTSTP
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while (keepRunning == 1)
	{
		// checks what background processes are still running
		isProcessRunning();					
		getInput();		
		// check if command is a built in command (exit, cd, status)
		doBuiltIns();
	}
}


// Catches SIGINT (CTRL+C). Parent process, child background processes all ignore SIGINT,
// while a child running as a foreground process will terminate itself upon receiving
// SIGINT. Number of the signal that killed this child process will be displayed after.
void catchSIGINT(int signo)
{
	printf("terminated by signal %d\n", signo);
	fflush(stdout);
}

// Catches SIGTSTP (CTRL+Z). Children running in foreground and background will ignore SIGTSTP,
// while the parent process running the shell will toggle foreground-only mode on and off.
// If on, turns off, if off, turns on. Foreground-only mode forces all new processes to be run in the
// foreground, even ones specified to be background processes with &.
void catchSIGTSTP()
{
	// if flag is false, that means that foreground-only mode is off and we turn it on (it is toggled)
	if (sigtstpFlag == false)
	{
		// resets sigNum in case of previous SIGTSTP
		sigNum = 0;
		backgroundFlag = false;
		// sets flag to true so we know we received the SIGTSTP
		sigtstpFlag = true;

		printf("Entering foreground-only mode (& is now ignored)\n");
		fflush(stdout);

		// keeps track of signal count
		sigNum++;								
	}
	// otherwise, foreground-only mode is on and we turn it off
	else
	{
		// resets the flag for later
		sigtstpFlag = false;					

		printf("Exiting foreground-only mode\n");
		fflush(stdout);

		// keeps track of signal count
		sigNum++;
	}
}


// checks on background processes to see which are still running, since
// parent runs asynchronously. Displays a termination message including
// pid if a process was terminated.
void isProcessRunning()
{
	int i;

	for (i = 0; i < numProcesses; i++)	
	{
		// if waiting
		if (waitpid(processes[i], &childExitMethod, WNOHANG) > 0)
		{
			// if child process was terminated
			if (WIFSIGNALED(childExitMethod))
			{
				// displays pid of child as well as terminating signal number
				printf("background pid terminated is %d\n", processes[i]);
				printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
			}
			// check if process was terminated normally
			if (WIFEXITED(childExitMethod))
			{
				// prints in this format as per sample program execution
				printf("exit value %d\n", WEXITSTATUS(childExitMethod));
			}
		}
	}
}


// Reads each line from the user, checks for & (run in background command),
// and $$ (can be expanded). Formats smallsh command line with ":" on each line.
void getInput()
{
	// holds 4 chars of input (need 5 to include 0/)
	char lineInput[5] = { 0 };

	// for required formatting
	printf(": ");
	// flush the output buffer each time print is called as per assignment Hints & Resources 
	fflush(stdout);
	fgets(input, sizeof(input), stdin);
	// removes new line character for nicer formatting
	removeEndChar(input);
	// stores input in lineInput variable
	strncpy(lineInput, input, 4);

	// if not output and & is present, run in background
	if (strcmp(lineInput, "echo") != 0)	
	{
		if (strchr(input, '&') != NULL)	
		{
			runInBackground();
		}
	}
	// see if command is SIGTSTP or needs to be expanded ($$ indicator)
	if (strstr(input, "TSTP") == NULL)
		if (strstr(input, "$$") != NULL)
			expand();
	
	// see if command is SIGTSTP and catch it if it is
	if (strstr(input, "TSTP") != NULL)
		catchSIGTSTP();	
}

