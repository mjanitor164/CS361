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


// Removes newline character for nicer look
void removeEndChar(char *removeItem)
{
	// change to null char
	removeItem[strcspn(removeItem, "\n")] = '\0';
}


// Checks to see if process is able to be run in the background,
// sets a flag if so
void runInBackground()
{
	char backgroundCommand[MAX_CMD_LENGTH] = { 0 };
	// input string needs to be modified to remove the & symbol (because background process)
	// as well as the space
	int size = (strlen(input) - 2);					
	// Runs in background if there's no SIGTSTP
	if(sigtstpFlag == false)
		backgroundFlag = true;
	// Adds this process into the backgroundCommand array minus last two characters
	strncpy(backgroundCommand, input, size);
	strcpy(input, backgroundCommand);
}

// Expands any instance of $$ in a command into the process ID of of the smallsh itself
// This copies the string and modifies it based on the length
void expand()
{
	char inputArray[MAX_CMD_LENGTH] = { 0 };
	// sets size to be length minus 2--removing "$$"
	int size = (strlen(input) - 2);
	// put this input into inputArray
	strncpy(inputArray, input, size);
	// put the input back into input now that it's correctly formatted
	strcpy(input, inputArray);
	// append the shell's pid
	sprintf(inputArray, "%d", getppid());
	// concat the pid to input
	strcat(input, inputArray);
}

// Expands $$ into a process like expand() does, but this function is called
// after the fork instead and uses the child's pid.
void expandFromChild()
{
	// input array
	char expandFromChild[MAX_CMD_LENGTH] = { 0 };
	//  modify length--must be size minus 11--this removes the signal and "$$"
	int size = (strlen(input) - 11);
	// put this input into array with correct size
	strncpy(expandFromChild, input, size);
	// puts this back into input now that it's correctly formatted
	strcpy(input, expandFromChild);
	// append the child's pid
	sprintf(expandFromChild, "%d", getpid());
	// conct the pid to input
	strcat(input, expandFromChild);
}


// Processes the build in shell commands (cd, status, exit), as well
// as comments (denoted by #)
void doBuiltIns()
{
	// Handles cd built-in
	if (strncmp(input, "cd", 2) == 0)
	{
		char directoryArray[MAX_CMD_LENGTH];
		char *newPath;
		// gets current directory
		getdirectoryArray(directoryArray, sizeof(directoryArray));
		// need to get current directory and add it to path
		newPath = strstr(input, " ");
		// we need to format a new path before we can use it
		if (newPath)
		{
			newPath++;
			strcat(directoryArray, "/");
			strcat(directoryArray, newPath);
			// change directory to that new path (now directoryArray)
			chdir(directoryArray);
		}
		// if not a new path, we just get the home directory
		else
		{
			chdir(getenv("HOME"));
		}
		// now we get the new current directory
		getdirectoryArray(directoryArray, sizeof(directoryArray));
		// output the directory (required to match assignment output formatting)
		printf("%s\n", directoryArray);
		fflush(stdout);
	}
	// handles status built-in
	else if (strcmp(input, "status") == 0)
	{
		// output the exit value of status (required to match assignment output formatting)
		printf("exit value %d\n", status);
		fflush(stdout);
	}
	// handles the exit built-in
	else if (strcmp(input, "exit") == 0)
	{
		// setting this variable to 0 will end the program
		keepRunning = 0;
	}
	// handles comments (denoted by # or a space)
	else if (strncmp(input, "#", 1) == 0 || strcmp(input, " ") == 0)
	{
		// do nothing
	}
	// if nothing else, we fork the process
	else
	{
		forkProcess();
	}

	// reset this flag in case it was flagged earlier
	backgroundFlag = false;							
}


// Forks the process to create a child process
// Contains logic to cause the parent to wait or continue
// when the child is made, depending on if the child is a 
// background or foreground process
void forkProcess()
{
	initPid = fork();

	// check to make sure it forked correctly (error returns negative number)
	if (initPid < 0)
	{
		printf("Error Forking\n");
		fflush(stdout);
		exit(1);
	}
	else if (initPid == 0)
	{
		// check for sigtstp flag
		if (sigNum > 0)	
		{
			// make sure there isn't a bounty out for the child process
			if (strstr(input, "kill") != NULL)
			{
				expandFromChild();
			}

		}
		// run commands for the child
		executeCommand();
	}
	/*************************************************************************************************************/
	else //PARENT
	{
		if (backgroundFlag == true)					//Check if child is running in the background
		{
			processes[numProcesses] = initPid;	//Add child to background array
			numProcesses++;						//Add one to the process counter
			waitpid(initPid, &childExitMethod, WNOHANG);	//Dont let the parent wait for child
			backgroundFlag = false;					//reset background flag

			printf("background pid is %d\n", initPid);		//Output the childs PID
			fflush(stdout);
		}
		else
		{
			waitpid(initPid, &childExitMethod, 0);	//Let the parent wait for child process to finish
			if (WIFEXITED(childExitMethod))			//Check if there was a problem 
				status = WEXITSTATUS(childExitMethod);	//Set status to error
		}
	}
}


// Once a child process is made, executeCommand runs the command.
// This reads the full command, opens or creates files if directed,
// sets pointers, and handles errors.
void executeCommand()
{
	char* commandArgv[512];
	int count = 0;
	int redirect = 0;
	int fileDesc;
	int i = 0;
	int std = 2; //FP Error

	commandArgv[0] = strtok(input, " ");			//Get the Bash command from file

	while (commandArgv[count] != NULL)				//While not at the end
	{
		count++;									//add one to count
		commandArgv[count] = strtok(NULL, " ");		//Add command to array
	}

	while (count != 0)								//Loop while there is still data
	{
		if (strcmp(commandArgv[i], "<") == 0)		//Check redirect
		{
			fileDesc = open(commandArgv[i + 1], O_RDONLY, 0);	//Open File ReadOnly
			if (fileDesc < 0)						//Check if open file was successful
			{
				printf("cannot open %s for input\n", commandArgv[i + 1]);	//Output error
				fflush(stdout);
				exit(1);
			}
			else
			{
				std = 0;//0 = stdin					//Set File Pointer to stdin
				redirect = 1;						//There is a redirect
			}
		}
		else if (strcmp(commandArgv[i], ">") == 0) //Check redirect
		{
			fileDesc = open(commandArgv[i + 1], O_CREAT | O_WRONLY, 0755);  //Create File in WriteOnly
			std = 1; //1 = stdout					//Set File Pointer to stdout
			redirect = 1;							//There is a redirect
		}

		if (redirect == 1) //Common output between < and >
		{
			dup2(fileDesc, std);
			commandArgv[i] = 0;						//Remove the redirection from array
			execvp(commandArgv[0], commandArgv);	//Call exec with based on redirect
			close(fileDesc);						//Close file
		}

		count--;									//Decrement counter
		i++;										//Move to next element
		redirect = 0;								//Reset redirect
		std = -2;									//Reset to FP error
	}
	if(status=execvp(commandArgv[0], commandArgv) != 0); //The command was not a redirect. Execute the command
	{
		printf("%s: no such file or directory\n", input);	//If error then ouput issue
		exit(status);										//exit with error
	}
}