/*********************************************
* CS344 - Assignment 3 - smallsh
* Author: Megan Janitor
* File: smallsh.c
* This program is used to create a shell.  It 
* some builtin commands for cd, status, exit, 
* and #.  If a command is not builtin then the
* program will pass the command to the unix
* shell with exec.
*********************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CHAR		2048
#define MAX_COMMAND		512

/*Prototpye initialization*/
void catchSIGINT(int signo);
void catchSIGTSTP();
void isRunningProcess();
void userInput();
void removeEndNewLine(char *removeItem);
void checkBackground();
void expand$$();
void killCommand();
void builtIns();
void forkProcesses();
void runCommands();

/*Global variables*/
int quit = 1;					//Exit the program 
int statCode;
int bgProcess[40];				//Array for background processes.
int bgProcessCounter;			//A counter for the amount of background processes.
char input[MAX_CHAR];			//Input array
bool flagBackground = false;	//Background flag
bool flagSIGTSTP = false;		//SIGTSTP flag
int sigCounter = 0;				//Counter for flagSIGTSTP. > 0 captured signal
pid_t spawnPid = -2;			//Initialize spanPid to something random.
int childExitMethod = -2;		//Initialize childExitMethod to something random.



void main()
{
	struct sigaction SIGINT_action = { 0 };		//SIGINT initialization
	struct sigaction SIGTSTP_action = { 0 };	//SIGTSTP initialization

	SIGINT_action.sa_handler = catchSIGINT;		//Handler to catch the SIGINT signal
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);	//Create sigaction for SIGINT

	SIGTSTP_action.sa_handler = catchSIGTSTP;	//Handler to catch the SIGTSTP signal
	sigfillset(&SIGTSTP_action.sa_mask);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	//Create sigaction for SIGTSTP

	while (quit == 1)
	{
		isRunningProcess();						//Check up on the background processes
		userInput();							//Call function to get console input
		builtIns();								//Call and check built in commands first
	}
}

/*******************************************
*				catchSIGINT
* This function is used to catch the SIGINT
* signal.  Insteal of termination the signal
* is ignored and a terminatation number is
* output to the user.
********************************************/
void catchSIGINT(int signo)
{
	printf("terminated by signal %d\n", signo);
	fflush(stdout);
}

/*******************************************
*				catchSIGTSTP
* This function is used to catch the SIGTSTP
* signal. The signal will disable background
* proccesses and display the new consition to
* the user.
********************************************/
void catchSIGTSTP()
{
	if (flagSIGTSTP == false)
	{
		sigCounter = 0;								//Set the SigCounter to 0. For new run
		flagBackground = false;						//Background commands will not work
		flagSIGTSTP = true;							//The shell recieved a signal

		printf("Entering foreground-only mode (& is now ignored)\n");
		fflush(stdout);

		sigCounter++;								//Add one to the counter
	}
	else  //Exiting foreground-only mode
	{
		flagSIGTSTP = false;						//Reset the flag

		printf("Exiting foreground-only mode\n");
		fflush(stdout);

		sigCounter++;								//Add one to the counter
	}
}

/*******************************************
*				isRunningProcess
* This function is used to check on the
* background processes.  The processes ID's
* were placed into an array.  When ran, the
* parent can see how the child exited.  This
* is because the parent does not wait on
* the background to finish.  If there is an
* terminated signal then it will be displayed
* here.
********************************************/
void isRunningProcess()
{
	int i;

	for (i = 0; i < bgProcessCounter; i++)				//Loop through each element
	{
		if (waitpid(bgProcess[i], &childExitMethod, WNOHANG) > 0)	//Parent process shouldn�t wait
		{
			if (WIFSIGNALED(childExitMethod))			//Check if child process terminated because it received a signal
			{
				printf("background pid terminated is %d\n", bgProcess[i]);			//Child PID	
				printf("terminated by signal %d\n", WTERMSIG(childExitMethod));		//Signal number
			}
			if (WIFEXITED(childExitMethod))				//Check if child process terminated normally
			{
				printf("exit value %d\n", WEXITSTATUS(childExitMethod));
			}
		}
	}
}

/*******************************************
*				userInput
* This function is used to print a : to the
* console.  It will also read each line from 
* the user or script file. This function also
* checks certain conditions to see if 
* a process is eligle to be ran in the 
* background and it the $$ can be expanded.
********************************************/
void userInput()
{
	char isOutput[5] = { 0 };						//Small array to hold the first 4 char of input

	printf(": ");
	fflush(stdout);
	fgets(input, sizeof(input), stdin);				//Grab input line
	removeEndNewLine(input);						//Remove Newline char

	strncpy(isOutput, input, 4);					//Get the first 4 chars in input

	if (strcmp(isOutput, "echo") != 0)				//Check if the input is output versus command
	{
		if (strchr(input, '&') != NULL)				//Check if this will run in the background
		{
			checkBackground();						//Call checkBackground
		}
	}
	if (strstr(input, "TSTP") == NULL)				//Check to make sure the command is not SIGTSTP
		if (strstr(input, "$$") != NULL)			//Check if command will be expanded
			expand$$();								//Call expand$$
	
	if (strstr(input, "TSTP") != NULL)				//Check to if command is SIGTSTP
		catchSIGTSTP();								//Catch the signal
}

/*******************************************
*			removeEndNewLine
* This function is used to remove the newline
* character at the end of each string.  This
* will make outputting more uniform.
********************************************/
void removeEndNewLine(char *removeItem)
{
	removeItem[strcspn(removeItem, "\n")] = '\0';	//Change the found newline character 
}

/*******************************************
*				checkBackground
* This function is used to see if the proccess
* will run in the background.  It will set the
* background flag and remove the & from the 
* string so that exec can accept the command.
********************************************/
void checkBackground()
{
	char backgroundCommand[MAX_CHAR] = { 0 };		//Create and initialize array to store input
	int size = (strlen(input) - 2);					//Get the length of input and reduce by 2 to remove & and space

	if(flagSIGTSTP == false)						//If there is no signal caught then we can enter background mode
		flagBackground = true;						//Run command in background
	
	strncpy(backgroundCommand, input, size);		//Copy input into backgroundCommand minus 2 chars
	strcpy(input, backgroundCommand);				//Copy backgroundCommand back to input with correct format
}

/*******************************************
*				expand$$
* This function is used to expand $$ into 
* a process.  This will copy the string and
* modify the string based on the lengeth.
* The $$ will be removed and then replaced
* with the shell PID.
********************************************/
void expand$$()
{
	char expandCommand[MAX_CHAR] = { 0 };			//Create and initialize array to store input
	int size = (strlen(input) - 2);					//Get the length of input and reduce by 2 to remove $$

	strncpy(expandCommand, input, size);			//Copy input into expandCommand minus 2 chars
	strcpy(input, expandCommand);					//Copy expandCommand back to input with correct format
	sprintf(expandCommand, "%d", getppid());		//Add Shell PID to the end of string
	strcat(input, expandCommand);					//Cat the modification to input
}

/*******************************************
*				killCommand
* This function is used to expand $$ into
* a process.  This uses the same logic as 
* expand$$, but instead of using the Shell 
* PID this is called after the fork and will 
* use the child PID.
********************************************/
void killCommand()
{
	char killCommand[MAX_CHAR] = { 0 };				//Create and initialize array to store input
	int size = (strlen(input) - 11);				//Get the length of input and reduce by 11 to remove signal and $$

	strncpy(killCommand, input, size);				//Copy input into killCommand minus 11 chars
	strcpy(input, killCommand);						//Copy killCommand back to input with correct format
	sprintf(killCommand, "%d", getpid());			//Add child PID to the end of string
	strcat(input, killCommand);						//Cat the modification to input
}

/*******************************************
*				builtIns
* This function is used to orginize and use 
* the built in shell commands.  Such as cd, 
* status, exit, and #.
********************************************/
void builtIns()
{
	if (strncmp(input, "cd", 2) == 0)				//CD command
	{
		char cwd[MAX_CHAR];							//Create directory array
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
		printf("exit value %d\n", statCode);		//Output the exit value of status
		fflush(stdout);
	}
	else if (strcmp(input, "exit") == 0)			//Exit command
	{
		quit = 0;									//End the program
	}
	else if (strncmp(input, "#", 1) == 0 || strcmp(input, " ") == 0)  //Comment command
	{
		//Do nothing if comment
	}
	else
	{
		forkProcesses();							//Call and get ready to fork
	}

	flagBackground = false;							//Reset background flag if flaged for built-in commands.
}

/*******************************************
*				forkProcesses
* This function is used to fork the program
* and creat child processes that can either
* run in the background or foreground.  
* These children are used to run unix 
* commands. Such as < > and more through 
* exec. Depending on how the child is 
* running, Background or Foreground, the
* parent will wait for the child or continue.
********************************************/
void forkProcesses()
{
	spawnPid = fork();								//Fork the process Create a child process

	if (spawnPid < 0)								//Check if the process forked without an error
	{
		printf("Error Forking\n");
		fflush(stdout);
		exit(1);
	}
	else if (spawnPid == 0)							//Process forked fine
	{
		if (sigCounter > 0)							//Check if SIGTSTP has been flagged
		{
			if (strstr(input, "kill") != NULL)		//Check if there is a kill command out for the child
			{
				killCommand();						//Call kill command to add child PID to input
			}

		}
		runCommands();								//Move on to commands for child
	}
	else //PARENT
	{
		if (flagBackground == true)					//Check if child is running in the background
		{
			bgProcess[bgProcessCounter] = spawnPid;	//Add child to background array
			bgProcessCounter++;						//Add one to the process counter
			waitpid(spawnPid, &childExitMethod, WNOHANG);	//Dont let the parent wait for child
			flagBackground = false;					//reset background flag

			printf("background pid is %d\n", spawnPid);		//Output the childs PID
			fflush(stdout);
		}
		else
		{
			waitpid(spawnPid, &childExitMethod, 0);	//Let the parent wait for child process to finish
			if (WIFEXITED(childExitMethod))			//Check if there was a problem 
				statCode = WEXITSTATUS(childExitMethod);	//Set statCode to error
		}
	}
}

/*******************************************
*				runCommands
* This function is used to control the child
* processes actions.  
********************************************/
void runCommands()
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
	if(statCode=execvp(commandArgv[0], commandArgv) != 0); //The command was not a redirect. Execute the command
	{
		printf("%s: no such file or directory\n", input);	//If error then ouput issue
		exit(statCode);										//exit with error
	}
}