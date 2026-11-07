#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>

char prompt[64] = "hello:";  // Holds the prompt, initialized to default - related to feature 1

// Helper function for feature 10 to catch and handle the SIGINT signal
void handle_sigint(int sig) {
    signal(SIGINT, handle_sigint);
    printf("\nYou typed Control-C!\n");
    printf("%s ", prompt); // Reprint the prompt so it remains visible
    fflush(stdout);
}

// Helper function for feature 12: catches the exit status of a background process and prints it
void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    pid = wait(&status);
    if (pid > 0) {
        printf("Child %d finished with exit status %d\n", pid, status >> 8);
        printf("%s", prompt);
        fflush(stdout);
    }
}

// General helper function to split a command into words and return the word count
int parse_args(char *line, char **argv) {
    int i = 0;
    char *p = line;

    while (*p != '\0') {
        // Skip spaces
        while (*p == ' '){
            p++;
        } 
        if (*p == '\0'){
            break;
        } 

        // If it starts with quotes, take everything inside as a single string
        if (*p == '"') {
            p++; // Skip the opening quote
            argv[i++] = p; // Start the string after the quote
            while (*p != '"' && *p != '\0') p++;  // Search for the closing quote
            if (*p == '"') *p++ = '\0';  // Replace the closing quote with \0
        } else {
            // If it's a regular word, split by spaces
            argv[i++] = p;
            while (*p != ' ' && *p != '\0') p++;
            if (*p == ' ') *p++ = '\0';
        }
    }
    argv[i] = NULL;  // Null-terminate the array
    return i;
}

int main() {
    char command[1024];            // The command entered by the user
    char lastCommand[1024] = "";   // The last executed command - related to features 4-5
    char *token;                   // A token from the split command
    char *outfile;                 // The target file for output redirection
    char *errfile;                 // The target file for error redirection - related to feature 7 
    char *appendfile;              // The target file for append redirection - related to feature 8
    char *infile;                  // The source file for input redirection - related to feature 9
    int i, fd, amper, retid, status;
    int redirect;                  // Indicator for output redirection
    int redirect_err;              // Indicator for error redirection - related to feature 7
    int redirect_app;              // Indicator for append redirection - related to feature 8
    int redirect_in;               // Indicator for input redirection - related to feature 9
    int last_status = 0;           // Exit status of the last executed command - related to feature 2
    char *argv[10];                // Array containing the command arguments

    // Signal handling
    signal(SIGINT, handle_sigint); // Implement feature 10
    signal(SIGCHLD, handle_sigchld);  // Implement feature 12

    while (1)
    {
        printf("%s ", prompt); // Print the prompt
        fflush(stdout); // Force prompt to print immediately (flush buffer)
        fgets(command, 1024, stdin); // Read input from the user
        command[strlen(command) - 1] = '\0'; // Replace the newline character with \0

        // Implement feature 5
        if (strcmp(command, "\x1B[A") == 0) {
            if (strlen(lastCommand) == 0) {
                continue; // If there is no previous command, do nothing
            }
            strcpy(command, lastCommand); // Copy the last executed command into 'command'
            printf("%s\n", command);  // Print the last command
        }

        // If the command is not Up-Arrow or "!!", save it - related to features 5-6
        if (strcmp(command, "!!") != 0 && strcmp(command, "\x1B[A") != 0){
            strcpy(lastCommand, command);
        }
            
        // Search for '|' in the command - primarily for feature 11
        char *commands[10]; // Initialize array for up to 10 sub-commands
        int num_commands = 0; // Count the number of sub-commands
        char *cmd = strtok(command, "|"); // Split the main command by '|' and get the first sub-command
        
        // Loop to continue splitting the command by '|'
        while (cmd != NULL) {
            commands[num_commands] = cmd;
            num_commands++;
            cmd = strtok(NULL, "|");
        }

        // Implement feature 11
        if (num_commands > 1) {
            int pipes[10][2]; // Create infrastructure for up to 10 commands

            // Check for redirection
            char *pipe_outfile = NULL; // Pointer to hold the target file if redirection exists at the end of the pipeline
            char *last_cmd = commands[num_commands - 1]; // Get the last command from the commands array
            char *redirect_pos = strstr(last_cmd, " > "); // Search for the redirection symbol and return a pointer if found
            if (redirect_pos != NULL) {
                *redirect_pos = '\0'; // If found, place \0 before the redirection symbol
                pipe_outfile = redirect_pos + 3; // Position the pointer to the target output file
            }

            // Pre-create the pipes for the sub-commands before calling fork
            for (int i = 0; i < num_commands - 1; i++) {
                pipe(pipes[i]);
            }

            // Create a child process for each sub-command
            for (int i = 0; i < num_commands; i++) {
                char *cmd_argv[10]; // Array to hold the sub-command arguments
                int k = 0; // Sub-command arguments counter
                char *t = strtok(commands[i], " "); // Split by spaces according to the found sub-commands array
                while (t != NULL) {
                    cmd_argv[k] = t;
                    t = strtok(NULL, " ");
                    k++;
                }
                cmd_argv[k] = NULL;

                if (fork() == 0) {
                    // Loop to close pipes irrelevant to this child process
                    for (int p = 0; p < num_commands - 1; p++) {
                        if (p != i - 1){
                            close(pipes[p][0]);
                        }

                        if (p != i){
                            close(pipes[p][1]);
                        }
                    }

                    // If not the first command, redirect input to read from the previous command's pipe
                    if (i > 0) {
                        close(STDIN_FILENO);
                        dup(pipes[i-1][0]);
                        close(pipes[i-1][0]);
                    }

                    // If not the last command, redirect output to write to the next command's pipe
                    if (i < num_commands - 1) {
                        close(STDOUT_FILENO);
                        dup(pipes[i][1]);
                        close(pipes[i][1]);
                    } else if (pipe_outfile != NULL) { // If it's the last command and there is redirection, write to the file
                        fd = creat(pipe_outfile, 0660);
                        close(STDOUT_FILENO);
                        dup(fd);
                        close(fd);
                    }

                    execvp(cmd_argv[0], cmd_argv); // Execute the current child's command
                }
            }

            // Close all pipes in the parent process
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Parent waits for all children to finish to ensure correct output order
            for (int j = 0; j < num_commands; j++) {
                wait(NULL);
            }

            continue;
        }

        i = parse_args(commands[0], argv); // Split command into words with quote support and save the word count

        // Check if the command is empty
        if (argv[0] == NULL){
            continue;
        }
            
        // Implement feature 1
        if (!strcmp(argv[0], "prompt") && !strcmp(argv[1], "=")) {
            strcpy(prompt, argv[2]); // Save the new prompt
            continue;
        }

        // Implement feature 2
        if (!strcmp(argv[0], "status")) {
            printf("%d\n", last_status); // Print the status of the last command
            continue;
        }

        // Implement feature 3
        if(!strcmp(argv[0], "cd")) {
            if(chdir(argv[1]) != 0){
                perror("cd failed"); // If the change directory operation failed
            }
        
            continue;
        }

        // Implement feature 4
        if (!strcmp(argv[0], "!!")) {
            if (strlen(lastCommand) == 0) {
                printf("No previous command\n"); // If there is no saved previous command
                continue;
            }
            strcpy(command, lastCommand); // Copy the last command into 'command'
            i = parse_args(command, argv); // Split the command into words and save the word count
        }

        // Implement feature 6
        if (!strcmp(argv[0], "quit")) {
            exit(0); // Exit the program
        }

        // Check for '&' at the end of input to run commands in the background - related to feature 12
        if (i >= 1 && !strcmp(argv[i - 1], "&")) {
            amper = 1; // Boolean flag set to true
            argv[i - 1] = NULL; // Remove it
            i--; // Decrease the word count
        }
        else
            amper = 0;

        // Handle output redirection
        if (i >= 2 && argv[0] && !strcmp(argv[i - 2], ">")) {
            redirect = 1; // Set the output redirection indicator
            outfile = argv[i - 1]; // Save the target output file name
            argv[i - 2] = NULL; // Clear unnecessary parts from the command
            i -= 2;
        }
        else
            redirect = 0;

        // Implement feature 7
        if (i >= 2 && argv[0] && !strcmp(argv[i - 2], "2>")) {
            redirect_err = 1;
            errfile = argv[i - 1]; // The last word (target file)
            argv[i - 2] = NULL; // Remove the redirection symbol
            i -= 2;
        }
        else
            redirect_err = 0;

        // Implement feature 8
        if (i >= 2 && argv[0] && !strcmp(argv[i - 2], ">>")) {
            redirect_app = 1; // Set the append redirection indicator
            appendfile = argv[i - 1]; // Save the target append file name
            argv[i - 2] = NULL; // Clear unnecessary parts from the command
            i -= 2;
        }
        else
            redirect_app = 0;

        // Implement feature 9
        if (i >= 2 && argv[0] && !strcmp(argv[i - 2], "<")) {
            redirect_in = 1; // Indicator for reading from another file
            infile = argv[i - 1]; // Save the source file name to read from
            argv[i - 2] = NULL; // Clear unnecessary parts from the command
            i -= 2;
        }
        else
            redirect_in = 0;

        // Create a child process to execute the command
        if (fork() == 0) {

            // If output redirection indicator is set
            if (redirect) {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }

            // If error redirection indicator is set - related to feature 7
            if (redirect_err) {
                fd = creat(errfile, 0660); // Create the error log file
                close(STDERR_FILENO); // Close standard error output
                dup(fd); // The new error file takes the place of STDERR
                close(fd); // Close the original error file descriptor
            }

            // If append redirection indicator is set - related to feature 8
            if (redirect_app) {
                fd = open(appendfile, O_WRONLY | O_CREAT | O_APPEND, 0660); // Open the file with write, create, and append permissions
                close(STDOUT_FILENO); // Close standard output
                dup(fd); // The append file takes the place of STDOUT
                close(fd); // Close the original file descriptor
            }

            // If input redirection indicator is set - related to feature 9
            if (redirect_in) {
                fd = open(infile, O_RDONLY); // Open the source file with read-only permissions
                if (fd < 0) {
                    perror("cannot open file");
                    exit(1);
                }
                close(STDIN_FILENO); // Close standard input
                dup(fd); // The input file takes the place of STDIN
                close(fd); // Close the original file descriptor
            }

            execvp(argv[0], argv); // Execute the command in the child process
        }

        // If the background execution indicator is set - related to feature 12
        if (amper == 0) { // If there was no '&' at the end of the command
            retid = wait(&status); // Parent waits for the child to finish; retid gets the PID and status gets the exit info
            last_status = WEXITSTATUS(status); // Extract the actual exit status - for feature 2
        }
    }
}