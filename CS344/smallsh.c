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

/*******************************************
*				getInput
* This function is used to print a : to the
* console.  It will also read each line from 
* the user or script file. This function also
* checks certain conditions to see if 
* a process is eligle to be ran in the 
* background and it the $$ can be expanded.
********************************************/
void getInput()
{
	char isOutput[5] = { 0 };						//Small array to hold the first 4 char of input

	printf(": ");
	fflush(stdout);
	fgets(input, sizeof(input), stdin);				//Grab input line
	removeEndChar(input);						//Remove Newline char

	strncpy(isOutput, input, 4);					//Get the first 4 chars in input

	if (strcmp(isOutput, "echo") != 0)				//Check if the input is output versus command
	{
		if (strchr(input, '&') != NULL)				//Check if this will run in the background
		{
			runInBackground();						//Call runInBackground
		}
	}
	if (strstr(input, "TSTP") == NULL)				//Check to make sure the command is not SIGTSTP
		if (strstr(input, "$$") != NULL)			//Check if command will be expanded
			expand();								//Call expand
	
	if (strstr(input, "TSTP") != NULL)				//Check to if command is SIGTSTP
		catchSIGTSTP();								//Catch the signal
}

/*******************************************
*			removeEndChar
* This function is used to remove the newline
* character at the end of each string.  This
* will make outputting more uniform.
********************************************/
void removeEndChar(char *removeItem)
{
	removeItem[strcspn(removeItem, "\n")] = '\0';	//Change the found newline character 
}

/*******************************************
*				runInBackground
* This function is used to see if the proccess
* will run in the background.  It will set the
* background flag and remove the & from the 
* string so that exec can accept the command.
********************************************/
void runInBackground()
{
	char backgroundCommand[MAX_CMD_LENGTH] = { 0 };		//Create and initialize array to store input
	int size = (strlen(input) - 2);					//Get the length of input and reduce by 2 to remove & and space

	if(sigtstpFlag == false)						//If there is no signal caught then we can enter background mode
		backgroundFlag = true;						//Run command in background
	
	strncpy(backgroundCommand, input, size);		//Copy input into backgroundCommand minus 2 chars
	strcpy(input, backgroundCommand);				//Copy backgroundCommand back to input with correct format
}

/*******************************************
*				expand
* This function is used to expand $$ into 
* a process.  This will copy the string and
* modify the string based on the lengeth.
* The $$ will be removed and then replaced
* with the shell PID.
********************************************/
void expand()
{
	char expandCommand[MAX_CMD_LENGTH] = { 0 };			//Create and initialize array to store input
	int size = (strlen(input) - 2);					//Get the length of input and reduce by 2 to remove $$

	strncpy(expandCommand, input, size);			//Copy input into expandCommand minus 2 chars
	strcpy(input, expandCommand);					//Copy expandCommand back to input with correct format
	sprintf(expandCommand, "%d", getppid());		//Add Shell PID to the end of string
	strcat(input, expandCommand);					//Cat the modification to input
}

/*******************************************
*				expandFromChild
* This function is used to expand $$ into
* a process.  This uses the same logic as 
* expand, but instead of using the Shell 
* PID this is called after the fork and will 
* use the child PID.
********************************************/
void expandFromChild()
{
	char expandFromChild[MAX_CMD_LENGTH] = { 0 };				//Create and initialize array to store input
	int size = (strlen(input) - 11);				//Get the length of input and reduce by 11 to remove signal and $$

	strncpy(expandFromChild, input, size);				//Copy input into expandFromChild minus 11 chars
	strcpy(input, expandFromChild);						//Copy expandFromChild back to input with correct format
	sprintf(expandFromChild, "%d", getpid());			//Add child PID to the end of string
	strcat(input, expandFromChild);						//Cat the modification to input
}

/*******************************************
*				doBuiltIns
* This function is used to orginize and use 
* the built in shell commands.  Such as cd, 
* status, exit, and #.
********************************************/
void doBuiltIns()
{
	if (strncmp(input, "cd", 2) == 0)				//CD command
	{
		char cwd[MAX_CMD_LENGTH];							//Create directory array
		char *newPath;
		getcwd(cwd, sizeof(cwd));					//Get the current directory

		newPath = strstr(input, " ");				//Get the cd and add to path
		if (newPath)
		{
			newPath++;
			strcat(cwd, "/");						//Cat / to end of current directory 
			strcat(cwd, newPath);					//Cat the new path and current directory
			chdir(cwd);								//Change directory
		}
		else
		{
			chdir(getenv("HOME"));					//Get home directory
		}

		getcwd(cwd, sizeof(cwd));					//Get the new current directory 
		printf("%s\n", cwd);						//Output the directory
		fflush(stdout);
	}
	else if (strcmp(input, "status") == 0)			//Status command
	{
		printf("exit value %d\n", status);		//Output the exit value of status
		fflush(stdout);
	}
	else if (strcmp(input, "exit") == 0)			//Exit command
	{
		keepRunning = 0;									//End the program
	}
	else if (strncmp(input, "#", 1) == 0 || strcmp(input, " ") == 0)  //Comment command
	{
		//Do nothing if comment
	}
	else
	{
		forkProcess();							//Call and get ready to fork
	}

	backgroundFlag = false;							//Reset background flag if flaged for built-in commands.
}

/*******************************************
*				forkProcess
* This function is used to fork the program
* and creat child processes that can either
* run in the background or foreground.  
* These children are used to run unix 
* commands. Such as < > and more through 
* exec. Depending on how the child is 
* running, Background or Foreground, the
* parent will wait for the child or continue.
********************************************/
void forkProcess()
{
	initPid = fork();								//Fork the process Create a child process

	if (initPid < 0)								//Check if the process forked without an error
	{
		printf("Error Forking\n");
		fflush(stdout);
		exit(1);
	}
	else if (initPid == 0)							//Process forked fine
	{
		if (sigNum > 0)							//Check if SIGTSTP has been flagged
		{
			if (strstr(input, "kill") != NULL)		//Check if there is a kill command out for the child
			{
				expandFromChild();						//Call kill command to add child PID to input
			}

		}
		executeCommand();								//Move on to commands for child
	}
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

/*******************************************
*				executeCommand
* This function is used to control the child
* processes actions.  
********************************************/
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