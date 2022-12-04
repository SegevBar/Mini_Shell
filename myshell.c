#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h> 

/* prototypes: */
pid_t my_fork();
void my_execvp(char** arglist);
int my_pipe(int[] pipefd);
int fork_and_execute(int count, char** arglist);
int execute_command_in_background(int count, char** arglist);
int find_pipe_symbol(char** arglist);
int execute_pipig_two_processes(int count, char** arglist, int pipe_symbol);

/*
* A function to execute shell command by the given symbols
* 
* This function receives:
*   -> count of none NULL elements in arglist
*   -> an array arglist with count non-NULL words. 
*      This array contains the parsed command line. 
*      The last entry in the array arglist[count] = NULL. 
*/
int process_arglist(int count, char** arglist) {
    int exec_result;
    int pipe_symbol;

    /* case "&" - run process in background */
    if ((strcmp(arglist[count-1], "&")) == 0) {
        exec_result = execute_command_in_background(count, arglist);
    } 
    /* case piping 2 processes */
    else if (pipe_symbol = find_pipe_symbol(arglist)) {
        exec_result = execute_pipig_two_processes(count, arglist, pipe_symbol);
    }
    return exec_result;
}

/*
* A function to initiate and setup shell
*/
int prepare(void) {
    signal(SIGINT, SIG_IGN);  /* After prepare() finishes, the parent (shell) should not terminate upon SIGINT */
    return 0;
}

/*
* This function executes before exiting shell
*/
int finalize(void) {
    return 0;
}



/*--------------------UTILS-FUNCTIONS-----------------------*/

/*
* A function that forks process and checks for errors
* returns pid value returned by fork(), and -1 if an error accured
*/
pid_t my_fork() {
    pid_t pid = fork();
    if (pid == -1) {
        /* failed to fork - raise error */
        perror("Failed executing fork");
        return pid;
    }
    return pid;
}

/*
* A function to execute shell command and checks for errors
*/
void my_execvp(char** arglist) {
    execvp(arglist[0], arglist);
    perror("Failed executing execvp");
    exit(1);
}

/*
* A function that creates a pipe and checks for errors
* returns -1 if error accurred
*/
int my_pipe(int[] pipefd) {
    int pipe_res = pipe(pipefd);

    if (pipe_res == -1) {
        perror("Failed executing pipe");
        return 0;
    }
    return 1;
}

/*
* A function to run shell command in the backgroud when "&" is the last arglist word.
*/
int execute_command_in_background(int count, char** arglist) {
    signal(SIGINT, SIG_IGN);  /* Background child processes should not terminate upon SIGINT */
    signal(SIGCHLD, SIG_IGN); /* ERAN'S TRICK for zombies */
    
    pid_t pid = my_fork();
    if (pid == 0) {
        /* child process */
        arglist[count-1] = NULL;  /* remove & from arglist */
        my_execvp(arglist);
    }
    return ((pid < 0) ? 0 : 1);
}

/*
* A function to search "|" word in arglist
* if "|" appears -> commands need to be piped
*
*/
int find_pipe_symbol(char** arglist) {
    int i = 0;
    has_pipe_sym = 0;

    while (arglist[i] != NULL) {
        if ((strcmp(arglist[i] ,"|")) == 0) {
            has_pipe_sym = i;  /* i != 0 according to description */
            break;
        } 
    }
    return has_pipe_sym;
}

/*
* A function that runs two child processes, with the output (stdout) of the 
* first process piped to the input (stdin) of the second process 
* 
*/
int execute_pipig_two_processes(int count, char** arglist, int pipe_symbol) {
    int pipefd[2];
    pid_t cpid;
    
    signal(SIGCHLD, SIG_IGN); /* ERAN'S TRICK for zombies */
    arglist[pipe_symbol] = NULL;

    if (!my_pipe(pipefd)) {  /* my_pipe returns 0 when error accures */
        return 0;
    }

    

  }

}
