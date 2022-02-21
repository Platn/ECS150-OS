#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <iostream>
#include <string>

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

string * parseCommand( string &input) {
	/* 1. Go in and count how many spaces there are for the # arguments
	** 2. Create an array of strings with the size of arguments
	** 3. Run through the string again and append all characters until a space is reached.
	** 4. If a space is reached, the next string in the area is moved to. Repeat the above process.
	** 5. Repeat the above two steps until all arguments are parsed and a newline has been found.
	** 6. Return this array once it has been filled. */
	
	int numArgs = 0;
	for(int i = 0; i < input.length(); i++) { // Search for the number of arguments
		if(input[i] == 0x20) // Find # of space
			numArgs++;
	}
	
	
	
	string *commandArgs = new string[numArgs + 1]; /* Create a string for the command and each argument
							* commandArgs[0] = command, commandArgs[1+] are arguments
							*/
	
	for(int i = 0; i < numArgs; i++) { // Set each string to be blank
		commandArgs[i] = "";
	}
	
	
	int argPos = 0;
	/* Read through the string once more, append to the string until it hits a
	 space, if so increment argPos */
	
	
	for(int i = 0; i < input.length(); i++) {
		if(input[i] != 0x20) { // Not equal to Space
			commandArgs[argPos] += input[i];
//			cout << input[i];
		}
		
		else {
			argPos++;
		}
	}
	
//    cout << "Num Args: " << numArgs << endl;
	
/*	 Debug Begin */
//    if(numArgs == 0) {
//        cout << "Command: " << commandArgs[0] << endl;
//    }
//    else {
//        for(int i = 0; i < numArgs + 1; i++) {
//            cout << "Arg" << i << ": " << commandArgs[i] << endl;
//        }
//    }
//
//	cout << "Args[0]: " << commandArgs[0] << " Args[1]" << commandArgs[1] << endl;
	
	/* Debug End */
	
	return commandArgs;
}

char * strToCharArray(string input) {
	char * arr = new char[input.length()];
	for(int i = 0; i < input.length(); i++) {
		arr[i] = input[i];
	}
	
	return arr;
}

void * pwd() {
	char * dir = getcwd(NULL, 0);
	size_t size = 0;
	while(1) {
		if(dir[size] == '\0')
			break;
		size++;
	}
	
//							char indChar;
	
	for(int i = 0; i < size; i++)
	{
//								char * indChar = &dir[i];
		
		write(STDOUT_FILENO, &dir[i], 1); // Write to console
//								indChar = NULL;
//								delete indChar;
	}
	
	
	
	
//							int fd = open("./ECS-150-Project1/file.txt", O_CREAT | O_RDWR);
	
	
//							ssize_t bytesWrote = write(fd, dir, size);
	
//							int closeFD = close(fd);
	
}


int main(int argc, char *argv[]){
    struct termios SavedTermAttributes;
    char RXChar;
	string userInput = "";

    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
	pwd();
	RXChar = '$';
	write(STDOUT_FILENO, &RXChar, 1);
	RXChar = 0x20;
	write(STDOUT_FILENO, &RXChar, 1);
    while(1){
        read(STDIN_FILENO, &RXChar, 1);
        if(0x04 == RXChar){ // C-d
            break;
        }
        else{
            if(isprint(RXChar)){
				write(STDOUT_FILENO, &RXChar, 1); // Write 1 byte at a time to display
//                printf("RX: '%c' 0x%02X\n",RXChar, RXChar);
				if(RXChar == 0x20) { // Space
					if(userInput.length() == 0) { // Do not add to the command, if it is blank
						continue;
					}
				}
				userInput += RXChar;
            }
            else{
				switch(RXChar) {
					case 0x7f: // Backspace
					case 0x08: // Other backspace
					{
						if(userInput.length() > 0)
							userInput.pop_back();
						
						/* Backspace + Space + Backspace to clear*/
						char backDelete = 0x08;
						write(STDOUT_FILENO, &backDelete, 1); // Write 1 byte at a time to display
						backDelete = 0x20;
						write(STDOUT_FILENO, &backDelete, 1); // Write 1 byte at a time to display
						backDelete = 0x08;
						write(STDOUT_FILENO, &backDelete, 1); // Write 1 byte at a time to display
						break;
					}
						
					case 0x0A: // Enter
					{
						char newLine = '\n';
						write(STDOUT_FILENO, &newLine, 1);
						string * cmdLine;
						cmdLine = parseCommand(userInput); // This function is not happy on CSIF
//						cout << "Commands: " << cmdLine[1] << endl;
						
						if(cmdLine[0].compare("cd") == 0)
						{
//							cout << "Inside cd" << endl;
							char * arg1 = strToCharArray(cmdLine[1]);
							
//							for(int i = 0; i < sizeof(arg1); i++)
//							{
//								char * indChar = &arg1[i];
//								write(STDOUT_FILENO, indChar, 1); // Write to console
////								indChar = NULL;
////								delete indChar;
//							}
							chdir(arg1);
							pwd();
							char newLine = '\n';
							write(STDOUT_FILENO, &newLine, 1);
						}
						else if(cmdLine[0].compare("ls") == 0)
						{
							
						}
						
						else if(cmdLine[0].compare("pwd") == 0)
						{
							pwd();
						}
						
						else if(cmdLine[0].compare("ff") == 0) {
							
						}
						else if(cmdLine[0].compare("exit") == 0) {
							exit(0); // Check with professor to find out which one
						}
						
						else {
//                            cout << "Invalid command." << endl; // Placeholder for now
						}
						
						userInput = "";
						break;
					}
				}
//                printf("RX: ' ' 0x%02X\n",RXChar); // Print Character
            }
        }
    }
    
    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}
