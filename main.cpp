#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>

using namespace std;

void ResetCanonicalMode(int fd, struct termios *savedattributes){
    tcsetattr(fd, TCSANOW, savedattributes);
}

void SetNonCanonicalMode(int fd, struct termios *savedattributes){
    struct termios TermAttributes;
    char *name;
    
    // Make sure stdin is a terminal. 
    if(!isatty(fd)){
        fprintf (stderr, "Not a terminal.\n");
        exit(0);
    }
    
    // Save the terminal attributes so we can restore them later. 
    tcgetattr(fd, savedattributes);
    
    // Set the funny terminal modes. 
    tcgetattr (fd, &TermAttributes);
    TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO. 
    TermAttributes.c_cc[VMIN] = 1;
    TermAttributes.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}

vector<string> parseArgs(string input) {
    vector<string> args;
    string singleArg = "";
    for(int i = 0; i < input.length(); i++) {
        if(input[i] == 0x20 || input[i] == 0x0a) { // Space or Enter
            args.push_back(singleArg);
            singleArg = "";
        }
        else {
            singleArg += input[i];
        }
    }

    return args;
}

void pwd() {
    char tmp[256];
    getcwd(tmp, 256);
    string parseDir = "";
    char newLine = 0x0a;
    for(int i = 0; i < 256; i++) {
        if(tmp[i] < 32 || tmp[i] > 126) {
            break;
        } 

        else {
            parseDir += tmp[i];
        }

    }
    
    for(int i = 0; i < parseDir.size(); i++) {
        write(STDOUT_FILENO, &tmp[i], 1);
    }
    write(STDOUT_FILENO, &newLine, 1);
}

void shellDir() {
    char tmp[256];
    getcwd(tmp, 256);
    string parseDir = "";
    char newLine = 0x0a, space = 0x20, symbol = 0x24;

    for(int i = 0; i < 256; i++) {
        if(tmp[i] < 32 || tmp[i] > 126) {
            break;
        } 

        else {
            parseDir += tmp[i];
        }

    }

    for(int i = 0; i < parseDir.size(); i++) {
        write(STDOUT_FILENO, &tmp[i], 1);
    }

    write(STDOUT_FILENO, &space, 1);
    write(STDOUT_FILENO, &symbol, 1);
    write(STDOUT_FILENO, &space, 1);

}

void bckSpc() {
    char backDelete = 0x08;
    write(STDOUT_FILENO, &backDelete, 1);
    backDelete = 0x20;
    write(STDOUT_FILENO, &backDelete, 1);
    backDelete = 0x08;
    write(STDOUT_FILENO, &backDelete, 1);
}

int main(int argc, char *argv[])
{
    struct termios SavedTermAttributes;
    char RXChar;
	string userInput = "";
    vector<string> upArrow, downArrow;

    shellDir();

    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);

    // We need printable, arrow keys and break statement

    while(1)
    { // Read in the characters individually
		read(STDIN_FILENO, &RXChar, 1); // Read in one at a time
        
		if(0x04 == RXChar){ // C-d
            printf("Here?");
			break;
		}

        else if(0x1b == RXChar) // Escape Key
        { 
            printf("Inside Escape");
            read(STDIN_FILENO, &RXChar, 1); // Bracket Key
			if(0x05B == RXChar) 
            { // Arrow Key
				read(STDIN_FILENO, &RXChar, 1);
				switch(RXChar) 
                {
					case 0x41: // Up Arrow
					{
                        if(upArrow.size() > 0) 
                        {
                            for(int i = 0; i < userInput.length(); i++) 
                            {
                                if(userInput[i] == 0x0a) // just in case of \n
                                    break;
                                bckSpc();
                            }

                            downArrow.push_back(userInput);
                            userInput = upArrow.back();
                            char tmpC;

                            for(int i = 0; i < userInput.length(); i++) 
                            { // Write out the entire line
                                if(userInput[i] == 0x0a) // Enter means stop
                                    break;
                                tmpC = userInput[i];
                                write(STDOUT_FILENO, &tmpC, 1);
                            }
                        }

                        else // Empty 
                        {
                            RXChar = '\a';
				            write(STDOUT_FILENO, &RXChar, 1);
                        }
                        // break;
                    }
                    

                    // case 0x42: // Down Arrow
                    // {
                        
                    //     break;
                    // }
                    
                }
            }
        }

        else 
        {
            if(isprint(RXChar)) 
            {
                write(STDOUT_FILENO, &RXChar, 1);
                userInput += RXChar;
            }

            else if(0x0a == RXChar) // Enter
            { 
                string tmp;
                while(downArrow.size() > 1) 
                { // Move everything from down to up
                    tmp = "";
                    tmp = downArrow.back();
                    upArrow.push_back(tmp);
                    downArrow.pop_back();
                }

                upArrow.push_back(userInput); // Do we do up arrow first 
                userInput = userInput + RXChar;

                vector<string> commandArgs;
                commandArgs = parseArgs(userInput);
                
                write(STDOUT_FILENO, &RXChar, 1); // Write out to the console

                if(commandArgs[0] == "cd") 
                {
                    int errorVal;
                    if(commandArgs.size() > 1) { // What do we do if 
                        errorVal = chdir(commandArgs[1].c_str()); // Changes directory to argument
                        if(errorVal == -1) {
                            perror("Directory could not be found."); // Change this message later
                        }
                        else {
                            pwd();
                        }
                    }

                    else 
                    {
                        errorVal = chdir(getenv("HOME"));
                        if(errorVal == -1) 
                        {
                            perror("cd "); // Change this message later
                        }

                        else 
                        {
                            pwd();
                        }
                    }
                }

                else if(commandArgs[0] == "ls") 
                {
                    if(commandArgs.size() > 1) // Go to location and list files
                    { 
                        // Here is where we probably need to pipe to send something to go look
                        // This will first have to create a pipe that will go to a new area and then
                        // retrieve the files/directories and return that string back
                        // There are probably other work arounds but we should do this one.
                        
                        // 1. Create two pipes
                        // 2. Use pipes for ls:
                        // 2a. Parent will wait for child
                        // 2b. Child is considered its own process, changes directory, returns string of files
                        // 2c. Parent uses string from pipe to print out list and information

                        int fd1[2]; // Stores both ends of Parent, read: 0 write: 1
                        int fd2[2]; // Stores both ends of Child, read: 0 write: 1
                        pid_t proc;

                        if(pipe(fd1) == -1) 
                        {
                            fprintf(stderr, "Pipe 1 failed"); // DEBUG: Change to write
                            break;
                        }

                        if(pipe(fd2) == -1) 
                        {
                            fprintf(stderr, "Pipe 2 failed"); // DEBUG: Change to write
                            break;
                        }

                        proc = fork();

                        if(proc < 0) 
                        { // Process did not fork
                            fprintf(stderr, "Fork failed"); // DEBUG: Change to write
                        }

                        else if(proc > 0) 
                        { // Parent Process enters here
                            // Does parent write the directory to the child and waits
                            // Child returns with string then finishes its process. But when does it terminate?

                            close(fd1[0]); // Close off reading side

                        }

                        else 
                        { // Child process enters here
                            // Goal is to go to directory and then 
                            close(fd1[1]); // Close off the writing end of Pipe 1
                        }

                    }

                    else 
                    { // List current working directory files
                        struct dirent *directEntry;
                        DIR *dirPtr = opendir(".");

                        if(dirPtr == NULL) 
                        {
                            perror("ls"); // Failed to find directory
                        }

                        while ((directEntry = readdir(dirPtr)) != NULL) 
                        {
                            printf("%s\n", directEntry->d_name); // Prints out all files, might need formatting
                        }
                    }
                }

                else if(commandArgs[0] == "pwd") 
                {
                    pwd(); // This might need to be changed later for pipe
                    // pwd might need a second function that does not print but returns a string
                }

                else if(commandArgs[0] == "ff") 
                {
                    // Similar to ls but it lists the paths to a file if it finds the name of it.
                    // i.e. ff(command) main.cpp(filename) ECS150(directory) will print the directory
                    // of any main.cpp in any subdirectories, starting in the given directory
                }

                else if(commandArgs[0] == "exit") 
                {
                    // Might need to reset some flags or deconstruct things here.
                    break; // This needs to go back into Canonical Mode
                }

                else 
                {
                    string notFound = "";
                    char newLine = 0x0a;
                    notFound = commandArgs[0] + " is not a command."; // This needs to be changed to actual prompt
                    for(int i = 0; i < notFound.size(); i++) 
                    {
                        write(STDOUT_FILENO, &notFound[i], 1);
                    }
                    write(STDOUT_FILENO, &newLine, 1);
                }
                shellDir(); // Print Directory after everything is done.
            }
        }

        userInput = "";
        
    }

    printf("Outside");

    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}
