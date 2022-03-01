#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <queue>


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


void writeC(string input) { // For debugging purposes
    // char newLine = '\n';
    // write(STDOUT_FILENO, &newLine, 1);
    for(int i = 0; i < input.length(); i++) {
        write(STDOUT_FILENO, &input[i], 1);
    }
    // write(STDOUT_FILENO, &newLine, 1);
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

void cd(vector<string> args) { // Change Directory
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

void ls(vector<string> args) {
    /* Lists the files/directories in the directory specified, if no directory is specified, 
    it lists the contents of the current working directory. The order of files listed does 
    not matter, but the type and permissions must precede the file/directory name with one
    entry per line. 

    It might be better or the right way to do things by writing things out to a file instead
    or that might just be part of exec() or pipe()? We need to find how to write to file.
    
    Are we allowed to use the regular file in, file out from the standard library? No the writing
    is meant to be '>' So that means we still need to have ls return something?
    */
    if(args.size() > 1) {
        DIR* dp = opendir(args[1].c_str());
        struct dirent* dirp;
        string fileName;
        while((dirp = readdir(dp)) != NULL) {
            fileName = dirp->d_name; // Convert directly to string from null-term char arr
            struct stat x;
            stat(dirp->d_name,&x);
            write(STDIN_FILENO,(S_ISDIR(x.st_mode)) ? "d" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IRUSR) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWUSR) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXUSR) ? "x" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IRGRP) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWGRP) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXGRP) ? "x" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IROTH) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWOTH) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXOTH) ? "x" : "-",1);
            writeC("\t");
            writeC(fileName); // Print for debug
            writeC("\n");
        }

        closedir(dp); // What if we want to store the strings? They need to be a vector of strings right?
    }

    else {
        // Current Directory
        char tmp[256];
        getcwd(tmp, 256);
        DIR* dp = opendir(tmp);
        struct dirent* dirp;
        string fileName;
        while((dirp = readdir(dp)) != NULL) {
            fileName = dirp->d_name; // Convert directly to string from null-term char arr
            struct stat x;
            stat(dirp->d_name,&x);
            write(STDIN_FILENO,(S_ISDIR(x.st_mode)) ? "d" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IRUSR) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWUSR) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXUSR) ? "x" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IRGRP) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWGRP) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXGRP) ? "x" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IROTH) ? "r" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IWOTH) ? "w" : "-",1);
            write(STDIN_FILENO,(x.st_mode && S_IXOTH) ? "x" : "-",1);
            writeC("\t");
            writeC(fileName); // Print for debug
            writeC("\n");
        }
        closedir(dp);
    }

    printf("\n");
}

class FileFinder {
    // Used for ff
    public:

    queue<string> directories;
    vector<string> fileMatches; // When string matches add here

    FileFinder(){}

    void startSearch(string searchTerm, string dirName) {
        char tmp[256];
        getcwd(tmp, 256);
        string holdDir(tmp);

        /* This part should be the root directory */
        directories.push(dirName);

        int rotate = 0;
        while(directories.size() > 0) {
            searchDir(searchTerm, directories.front());
            directories.pop();
            rotate++;
        }


        while (fileMatches.size() > 0) {
            writeC(fileMatches.back());
            writeC("\n");
            fileMatches.pop_back();
        }

        chdir(holdDir.c_str());

    }

    void searchDir(string searchTerm, string dirName) {
        string openDir = dirName;
        chdir(openDir.c_str());
        DIR* dp = opendir(openDir.c_str());
        struct dirent* dirp;
        string fileName;
        string fileDir;

        while((dirp = readdir(dp)) != NULL) {
            fileName = dirp->d_name; 

            if(fileName.compare(".") == 0 || fileName.compare("..") == 0) {
            continue;
            }
            struct stat x;
            stat(dirp->d_name,&x);
            fileDir = openDir + "/" + fileName;
        
            switch (x.st_mode & S_IFMT) {
                case S_IFDIR:{  // Directory detected
                    directories.push(fileDir);
                    break;
                }
          
                case S_IFREG: {
                    if(fileName.compare(searchTerm) == 0) {
                    fileMatches.push_back(fileDir);
                    }
                    break;
                }
            }
        }

        closedir(dp);
        dp = NULL;
        delete dp;
    }

  ~FileFinder() {}
};

void ff(vector<string> args) {
    /* Prints the path to all files matching the filename parameter in the directory (or subdirectories)
    specified by the directory parameter. If no directory is specified the current working directory is used.
    The order of the files listed does not matter. */

    FileFinder findFiles;
    findFiles.startSearch(args[1], args[2]);
    
}



bool runCmd(vector<string> args) { // These functions will need to have string return types
    if(args[0].compare("pwd") == 0) {
        pwd();
    }

    else if(args[0].compare("ls") == 0) {
        ls(args);
    }

    else if(args[0].compare("cd") == 0) {
        cd(args);
    }

    else if(args[0].compare("ff") == 0) {
        ff(args);
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
                                isRunning = runCmd(cmdArgs); // Commands might need to be run from front to back
                                // Sort would take in a string from ls, ls is run first but read in last with pipe
                                // Two scenarios: Run into pipe, no pipe ran.
                                // '>' means to overwrite files that already exist in the directory
                                // So what we do in ls is return the directory as a string and where the command
                                // that called it will be the one to print it out or pass it

                                // Second argument can either be a flag, a directory or even a pipe/symbol
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