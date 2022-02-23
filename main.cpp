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

vector<string> parseArgs(string input) { // Keep in mind: ">" and "|"
    vector<string> args;
    string singleArg = "";
    
    for(int i = 0; i < input.length(); i++) {
        if(input[i] == 0x20 || input[i] == 0x0a) { // Space or Enter
            if(singleArg.compare("") != 0) // In case of a bunch of spaces
            {
                args.push_back(singleArg);
            }
            
            
            singleArg = "";
        }
        else {
            singleArg += input[i];
        }
    }
    
    if(singleArg.length() != 0) { // Push the last one if it exists
        args.push_back(singleArg);
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

void cd(vector<string> args) {
    int errorVal;
    if(args.size() > 1) {
        errorVal = chdir(args[1].c_str()); // Changes directory to argument
        if(errorVal == -1) {
            perror("Directory could not be found."); // Change this message later
        }
        else {
            pwd();
        }
    }

    else // Only CD has been input
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

bool runCmd(vector<string> args) {
    if(args[0].compare("pwd") == 0) {
        pwd();
    }

    else if(args[0].compare("ls") == 0) {

    }

    else if(args[0].compare("cd") == 0) {
        
    }

    else if(args[0].compare("exit") == 0) {
        return false;
    }

    else {
        string error = "command not found: " + args[0] + "\n";
        for(int i = 0; i < error.length(); i++) {
            write(STDOUT_FILENO, &error[i], 1);
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    struct termios SavedTermAttributes;
    char RXChar;
	string userInput = "";
    vector<string> upArrow, downArrow;
    bool isRunning = true; 

    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    
    shellDir();
    // We need printable, arrow keys and break statement

    while(isRunning) { // Read in the characters individually
		read(STDIN_FILENO, &RXChar, 1); // Read in one at a time
        
		if(0x04 == RXChar) {
            break;
        }

        else if(0x01B == RXChar) { // Escape Key
            read(STDIN_FILENO, &RXChar, 1);
            if(0x05B == RXChar) {
                read(STDIN_FILENO, &RXChar, 1);
                switch(RXChar) {
                    case 0x41: { // Up Arrow
                        if(upArrow.size() > 0) {

                            downArrow.push_back(userInput);

                            for(int i = 0; i < userInput.length(); i++) { // Clear input
                                bckSpc();
                            }

                            userInput = upArrow.back();

                            for(int i = 0; i < userInput.length(); i++) { // Replace input with next command
                                write(STDOUT_FILENO, &userInput[i], 1);
                            }

                            upArrow.pop_back();
                        }

                        else {
                            char bell = '\a'; // Bell
                            write(STDOUT_FILENO, &bell, 1);
                        }

                        break;
                    }

                    case 0x42: { // Down Arrow
                        if(downArrow.size() > 0) {
                            for(int i = 0; i < userInput.length(); i++) { // Clear input
                                bckSpc();

                            }

                            upArrow.push_back(userInput);
                            userInput = downArrow.back();

                            for(int i = 0; i < userInput.length(); i++) {
                                write(STDOUT_FILENO, &userInput[i], 1);
                            }

                            downArrow.pop_back();
                        }

                        else {
                            char bell = '\a'; // Bell
                            write(STDOUT_FILENO, &bell, 1);
                        }
                        break;
                    }

                }

            }
        } // End of Arrow Keys

        else {
            if(isprint(RXChar)) {
                // Printable Characters
                write(STDOUT_FILENO, &RXChar, 1);
                userInput += RXChar;
            }

            else {
                switch(RXChar) {
                    case 0x7f:
                    case 0x08: { // Backspace
                        
                        if(userInput.length() > 0) {
                            userInput.pop_back();
                            bckSpc();
                        }
                        
                        else { // Cannot backspace
                            char bell = '\a'; // Bell
                            write(STDOUT_FILENO, &bell, 1);
                        }
                        break;
                    }

                    case 0x0A: { // Return(Enter) case
                        char newline = '\n';
                        write(STDOUT_FILENO, &newline, 1);

                        if(userInput.compare("") != 0) { // If empty string submitted
                            vector<string> cmdArgs;
                            cmdArgs = parseArgs(userInput);
                            
                            if(cmdArgs.size() != 0) {
                                isRunning = runCmd(cmdArgs);
                            }
                           
                            if(!isRunning) { // We might have to change how this exits depending on child. Not sure.
                                break;
                            }

                            string tmp;
                            
                            while(downArrow.size() > 1) { 
                                // Put everything back into history except for command not submitted
                                tmp = downArrow.back();
                                upArrow.push_back(tmp);
                                downArrow.pop_back();
                            }

                            if(downArrow.size() == 1) {
                                downArrow.pop_back();
                            }

                            // First we need to clear out the down arrow first right? Check if 
                            upArrow.push_back(userInput);


                            userInput = ""; // Reset once done
                        }

                        // Parse arguments into individual commands

                        shellDir(); // Print current directory again
                        break;
                    }
                }
            }
        } // End for Print Char and Enter
    } // End of While Loop

    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}