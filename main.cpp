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
#include <sys/wait.h>

using namespace std;

/* Things to complete:
    1. Sixteen character limit for directory.
    2. Pipe: " | "
    3. Exec ???
    4. Write to file " > "
*/

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

queue<string>* parseArgs(string input) { // Keep in mind: ">" and "|"
    queue<string>* args;
    args = new queue<string>;
    string singleArg = "";
    
    for(int i = 0; i < input.length(); i++) {
        if(input[i] == 0x20 || input[i] == 0x0a) { // Space or Enter
            if(singleArg.compare("") != 0) // In case of a bunch of spaces
            {
                args->push(singleArg);
            }
            
            
            singleArg = "";
        }

        else {
            singleArg += input[i];
        }
    }
    
    if(singleArg.length() != 0) { // Push the last one if it exists
        args->push(singleArg);
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

void pwd(queue<string>* args) { // In here, we might need to pop the queue itself.
    args->pop(); // Take pwd out of the vector
    // string argSize = "Args Size: " + to_string(args->size()) + "\n";
    // writeC(argSize);

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
   
    if (args->size() > 0) {
        if (args->front().compare("|")) { // Now we need to create parent and child
            args->pop(); // How do we account for if someone puts pipe but no argument after? This will have to be by professor
            // Begin the fork here where the child breaks off and starts running other apps
            pid_t pid;
            int fd1[2];
            int fd2[2];


            if (pipe(fd1) == -1) {
                string error = "\nPipe Failed";
                writeC(error);
                return;
            }

            if(pipe(fd2) == -1) {
                string error = "\nPipe Failed";
                writeC(error);
                // It should probably just print or return here.
                return;
            }

            parseDir += "\n";

            pid = fork();
            
            if (pid < 0) {
                writeC("\nFork Failed");
            }

            else if (pid > 0) {
                // Parent Process
                close(fd1[0]); // Close reading end of first pipe

                write(fd1[1], parseDir.c_str(), parseDir.length());
                close(fd1[1]); // Close writing end of pipe 1;

                wait(NULL); // Wait for child to send string

                close(fd2[1]); // Close the writing end of pipe 2;
                char readChar = '\0';
                string output = "";
                while(readChar != '\n') {
                    // We need to remember to add a '\n' when child process writes.
                    read(fd2[0], &readChar, 1);
                    output += readChar;
                }
                
                output = "Output: " + output;
                writeC(output);
                close(fd2[0]); // Close the reading end of pipe 2;
                // All of the pipe has been read in here, now what?
                
            }

            else {
                // Child Process
                // Whenever a child process is created, do we add an exit to the vector so that it can finish executing? Seems so.

                close(fd1[1]); // Close the writing end of pipe 1

            }

        }

        else {
            // This else statement might not be needed and can be merged with below, wait to see?
            for(int i = 0; i < parseDir.size(); i++) {
                write(STDOUT_FILENO, &tmp[i], 1);
            }
            write(STDOUT_FILENO, &newLine, 1);
        }
    }

    else {
        // Only pwd has been put through, so print it out immediately.
        for(int i = 0; i < parseDir.size(); i++) {
            write(STDOUT_FILENO, &tmp[i], 1);
        }
        write(STDOUT_FILENO, &newLine, 1);
    }
    
    // argSize = "Args Size Fin: " + to_string(args->size()) + "\n";
    // writeC(argSize);
}

void cd(queue<string>* args) { // Change Directory
    int errorVal;
    if(args->size() > 1) {
        errorVal = chdir(args->front().c_str()); // Changes directory to argument
        if(errorVal == -1) {
            perror("Directory could not be found."); // Change this message later
        }
        else {
            pwd(args);
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
            pwd(args);
        }
    }

}

void ls(queue<string>* args) {
    /* Lists the files/directories in the directory specified, if no directory is specified, 
    it lists the contents of the current working directory. The order of files listed does 
    not matter, but the type and permissions must precede the file/directory name with one
    entry per line. 

    It might be better or the right way to do things by writing things out to a file instead
    or that might just be part of exec() or pipe()? We need to find how to write to file.
    
    Are we allowed to use the regular file in, file out from the standard library? No the writing
    is meant to be '>' So that means we still need to have ls return something?
    */
    // ls will still need to be worked on to implement the pipe

    args->pop();
    if(args->size() > 1) {
        DIR* dp = opendir(args->front().c_str());
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
        // Start search seems to be the part that might not be necessary if we do fork();
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

    void forkSearch(string fileName, string dirName) {
        int fd1[2];
        int fd2[2];

        pid_t pid;

        // Early initialization is fine, we are just going to have the child only fork when it hits another directory,
        // When it calls on ff again, then it will automatically create a new directory, which works out. 

        if(pipe(fd1) == -1) {
            writeC("\nPipe1 Failed\n");
        }

        if(pipe(fd2) == -1) {
            writeC("\nPipe2 Failed\n");
        }

        pid = fork();

        if (pid < 0) {
            writeC("\nFork failed\n");
        }

        else if (pid > 0) {
            // Parent Process
            // Waits until child finishes for total output

            // Steps for writing to child
            // close(fd1[0]); // Close reading
            // write(fd1[1], fileName.c_str(), fileName.length());

            // Writing to child might not even be necessary.
            close(fd1[1]);
            wait(NULL); // Wait for child to finish

            close(fd2[1]); // Close writing of fd2
            vector<string> finalOut;

            char readIn = 'a'; // a is place holder
            string inputCat = "";
            
            while(readIn != '\0') {
                read(fd2[0], &readIn, 1);
                // Run until null character found
                if(readIn != '\n') {
                    inputCat += readIn;
                    
                }

                else {
                    inputCat += '\n';
                    finalOut.push_back(inputCat);
                    inputCat = "";
                }
            }

            close(fd2[0]);

            while (finalOut.size() > 0) {
                writeC(finalOut.back());
                finalOut.pop_back();
            }
        }

        else {
            // Child process id will be 0

            close(fd2[0]); // Writing only

            // Navigate to directory and open it
            string openDir = dirName;
            chdir(openDir.c_str());
            DIR* dp = opendir(openDir.c_str());
            struct dirent* dirp;
            string fileSearch;
            string fileDir;

            while((dirp = readdir(dp)) != NULL) {
                fileSearch = dirp->d_name; 

                if(fileSearch.compare(".") == 0 || fileSearch.compare("..") == 0) {
                    continue;
                }

                struct stat x;
                stat(dirp->d_name,&x);
                fileDir = openDir + "/" + fileSearch;
        
                switch (x.st_mode & S_IFMT) {
                    case S_IFDIR:{  // Directory detected
                        // directories.push(fileDir);

                        forkSearch(fileName, fileDir);
                        break;
                    }
          
                    case S_IFREG: {
                        if(fileName.compare(fileSearch) == 0) {
                            // File match
                            fileDir = fileDir + "\n";
                            write(fd2[1], fileDir.c_str(), fileDir.length());
                        }
                        break;
                    }
                }
            }


            char term = '\0';
            write(fd2[1], &term, 1);
            close(fd2[1]);

            closedir(dp);
            dp = NULL;
            delete dp;

            exit(0); // Might need this

        }

        
    }

  ~FileFinder() {}
};

void ff(queue<string>* args) {
    /* Prints the path to all files matching the filename parameter in the directory (or subdirectories)
    specified by the directory parameter. If no directory is specified the current working directory is used.
    The order of the files listed does not matter. */
    // We need to validate that ff has args, that's after implementing the queue.

    // args->pop();
    FileFinder findFiles;
    if(args->size() == 0) {
        writeC("\nError: No file specified."); // Placeholder
        return;
    }

    string fileName = args->front();
    // writeC("\nFilename: " + fileName);
    args->pop();
    string dirName;
    
    if(args->size() == 0) {
        char tmp[256];
        getcwd(tmp, 256);
        dirName = tmp;
    }

    else {
        dirName = args->front();
        args->pop();
    }
    
    
    // writeC("\nDirname: " + dirName);
    
    findFiles.startSearch(fileName, dirName); // It allows access to these
    // For this it would just be pop arg pop arg right if it were a queue?

    
}

void ff2(queue<string>* args) {

    FileFinder findFiles;
    string fileName = args->front();
    // writeC("\nFilename: " + fileName);
    args->pop();
    string dirName = args->front();
    // writeC("\nDirname: " + dirName);
    // writeC("\n");
    args->pop();
    // return;
    findFiles.forkSearch(fileName, dirName); // It allows access to these 
}

bool runCmd(queue<string>* args) { // These functions will need to have string return types
    // Each command will have its own designation of when to pop and when not to.
    while(args->size() > 0) {
        if(args->front().compare("pwd") == 0) {
            // Pwd probably needs to be reconstructed since it can be piped into anything
            pwd(args);
            // args->pop();
            // writeC("Arg Size Outer: " + to_string(args->size()));
        }

        else if(args->front().compare("ls") == 0) {
            // Our test will be print working directory into ls, pwd | ls, will bring out current dir
            ls(args);
        }

        else if(args->front().compare("cd") == 0) {
            cd(args);
        }

        else if(args->front().compare("ff") == 0) {
            args->pop();
            ff2(args); // Change to 1 for non fork
            while (args->size() > 0) {
                args->pop(); // This is a huge placeholder to clear everything, remember to remove
            }
        }

        else if(args->front().compare("exit") == 0) { // This is probably the only thing that doesn't need pipe
            return false;
        }

        else {
            string error = "command not found: " + args->front() + "\n";
            for(int i = 0; i < error.length(); i++) {
                write(STDOUT_FILENO, &error[i], 1);
            }
            args->pop();
        }
    }

    return true;
}


int main(int argc, char *argv[]) {
    struct termios SavedTermAttributes;
    char RXChar;
	string userInput = "";
    vector<string> upArrow, downArrow, history;
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
                        if(history.size() > 0) {
                            // It must have some sort of history.
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

                            else if(upArrow.size() == 0 && downArrow.size() > 0) {
                                // Down arrow holds all of it
                                char bell = '\a'; // Bell
                                write(STDOUT_FILENO, &bell, 1);
                            }

                            else {
                                // UpArrow has size of 0 and history will fill it is the goal

                                for(int i = 0; i < history.size(); i++) {
                                    upArrow.push_back(history[i]);
                                }

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
                            queue<string>* cmdArgs;
                            cmdArgs = new queue<string>;
                            cmdArgs = parseArgs(userInput);
                            // Push to history and clear out both arrows
                            history.push_back(userInput); 
                            while(upArrow.size() > 0) {
                                upArrow.pop_back();
                            }

                            while(downArrow.size() > 0) {
                                downArrow.pop_back();
                            }
                            
                            
                            if(cmdArgs->size() != 0) {
                                isRunning = runCmd(cmdArgs); // Commands might need to be run from front to back
                                // Sort would take in a string from ls, ls is run first but read in last with pipe
                                // Two scenarios: Run into pipe, no pipe ran.
                                // '>' means to overwrite files that already exist in the directory
                                // So what we do in ls is return the directory as a string and where the command
                                // that called it will be the one to print it out or pass it

                                // Second argument can either be a flag, a directory or even a pipe/symbol
                            }
                           
                            if(!isRunning) { // We might have to change how this exits depending on child. Not sure.
                                ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
                                exit(0);
                                break;
                            }

                            
                            // First we need to clear out the down arrow first right? Check if 

                            
                            userInput = ""; // Reset once done
                            cmdArgs = NULL;
                            delete cmdArgs;
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

/*
    *change vector<string> args to queue<string> args ? 
    If so, change FileFind arguments to match, something like
    string arg1 = args.front(); args.pop(); string arg2 = args.front(); args.pop();
    runCmd() will have to compare args.front() and then once inside it will pop off the command
    
    
    Our idea for "|" is that we keep passing the queue<string> args to each command and it will pop off a command
    after it enters, then inside of each command it will see what the arguments that are left. For example:
    1. ls | less, in this case it will enter ls, then pop the front
        1a. if "|" is detected, then it will default to current directory and append string to a holdString vector.
        1b. if argument is detected
            - Use the directory that is listed and then check if the next value is "|",
                - If "|" then do not print, append string to holdString Vector
                - Otherwise just print out whatever it is
        Now that ls is popped off, we read in the pipe and know to check the holdString for arguments
        Pop off "|", set bool redirect to true, then enter the next function. Function will pop and check
        If redirect is true then it will use the holdString as arguments instead of what is in the args
        
    Our current plan is to work on FF to get the fork() to work and the pipe to work.
    Where our problem here is, how do we do the parsing because each part receives its own variables.
    What if we don't worry about the parsing bit for FF outside of getting into FF. Ideally it just takes in
    two arguments string fileName and directory. With fork, we will have the child run the new directory, does that mean
    every time a directory is found, a new fork is created? That is probably it. Each time a new directory is found, fork,
    create a pipe between the two and the child basically returns a string to the parent, parent concatenates that. Now
    what gets passed to the child? With ff, if we already have the command running, then where do we start with child?

    Fork() occurs when a new directory is found and to be explored, so that means we pass the child the directory to cd into
    it then will list the directories itself and will fork() again if it finds another. It will be going through depth first search.
    So that means after a directory is found, it will append that to a vector of directory or file. First directory found, create child,
    child will cd into the next directory found and then do a list of directories and files there, if it finds a new directory, that child
    will create a new child, it repeats this process until no directories are found. It will close and then write to the parent, where
    the parent has a list of matches, it moves to the next directory. 
    Same concept as before. From left to right, fork when a new directory is hit, child will scan the directory below parent, 
    repeat until no directory found, child pipes the string to parent and parent adds that to their list of matches.

    After a child returns with results, parent moves to the next directory if it exists, if not, it returns until no child process,
    then the last parent process prints everything else out. I think that us using a vector is fine. We are just reading through
    anyways right? Well the question is more for when we don't use queue.


    
    

    Parent
        ^ Child pipes
            ^ Child pipes
                ^ Child pipes

*/