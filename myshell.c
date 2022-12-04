#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h> 
#include <errno.h>

/* prototypes: */
void sigchld_ignore_handler();
void sigchld_default_handler();
void sigint_ignore_handler();
void sigint_default_handler();
pid_t my_fork();
void my_execvp(char** arglist);
int my_pipe(int pipefd[]);
void my_dup2(int pipefd[], int i);
int fork_and_execute(int count, char** arglist);
int execute_command_in_background(int count, char** arglist);
int find_pipe_symbol(char** arglist);
int execute_pipig_two_processes(int count, char** arglist, int pipe_symbol);
int execute_redirect_stdout_to_file(int count, char** arglist);
int execute_cmd_regulary(int count, char** arglist);
int execute_cmd_regulary(int count, char** arglist);

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
    int exec_result = 0;
    int pipe_symbol;
    
    sigint_ignore_handler();  /* shell not terminate upon SIGINT */

    /* case "&" - run process in background */
    if ((strcmp(arglist[count-1], "&")) == 0) {
        exec_result = execute_command_in_background(count, arglist);
    } 
    /* case piping 2 processes */
    else if ((pipe_symbol = find_pipe_symbol(arglist))) {
        exec_result = execute_pipig_two_processes(count, arglist, pipe_symbol);
    }
    /* case redirating standart output to output file */
    else if ((strcmp(arglist[count-2], ">")) == 0) {
        exec_result = execute_redirect_stdout_to_file(count, arglist);
    } 
    /* case of cmd without symbol - regular execution */
    else {
        exec_result = execute_cmd_regulary(count, arglist);
    }
    return exec_result;
}

/*
* A function to initiate and setup shell
*/
int prepare(void) {
    sigint_ignore_handler();  /* After prepare() finishes, the parent (shell) should not terminate upon SIGINT */
    sigchld_default_handler();  /* the default behavior of SIGCHLD */
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
* A function to handle SIGCHLD signal with SIG_IGN
* Ignored signal behavior
* helped here - recitation3 signal_handler.c code
*/
void sigchld_ignore_handler() {
    struct sigaction new_action = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART | SA_NOCLDWAIT
    };
    
    if (sigaction(SIGCHLD, &new_action, NULL) == -1) {
        perror("Failed executing SIGCHLD SIG_IGN sigaction");
        exit(1);
    }
}

/*
* A function to handle SIGCHLD signal with SIG_DFL
* Default signal behavior
* helped here - recitation3 signal_handler.c code
*/
void sigchld_default_handler() {
    struct sigaction new_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_RESTART | SA_NOCLDWAIT
    };
    
    if ((sigaction(SIGCHLD, &new_action, NULL) == -1) && (errno != ECHILD) && (errno != EINTR)) {
        perror("Failed executing SIGCHLD SIG_DFL sigaction");
        exit(1);
    }
}

/*
* A function to handle SIGINT signal with SIG_IGN
* Ignored signal behavior
* helped here - recitation3 signal_handler.c code
*/
void sigint_ignore_handler() {
    struct sigaction new_action = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART
    };  
    
    if (sigaction(SIGINT, &new_action, NULL) == -1) {
        perror("Failed executing SIGINT SIG_IGN sigaction");
        exit(1);
    }
}

/*
* A function to handle SIGINT signal with SIG_DFL
* Default signal behavior
* helped here - recitation3 signal_handler.c code
*/
void sigint_default_handler() {
    struct sigaction new_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_RESTART
    };
    
    if (sigaction(SIGINT, &new_action, NULL) == -1) {
        perror("Failed executing SIGINT SIG_DFL sigaction");
        exit(1);
    }
}



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
int my_pipe(int pipefd[]) {
    int pipe_res = pipe(pipefd);

    if (pipe_res == -1) {
        perror("Failed executing pipe");
        return 0;
    }
    return 1;
}

/*
* A fiunction that execute dup2 and checks for errors
*/
void my_dup2(int pipefd[], int i) {
    int dup2_res = dup2(pipefd[i], i);
    
    if(dup2_res == -1) {
        perror("Failed executing dup2");
        exit(1);
    }
}

/*
* A function to run shell command in the backgroud when "&" is the last 
* arglist word.
*/
int execute_command_in_background(int count, char** arglist) {
    pid_t pid;
    
    pid = my_fork();
    if (pid == 0) {
        /* child process */
        sigchld_ignore_handler(); /* ERAN'S TRICK for zombies */
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
    int has_pipe_sym = 0;

    while (arglist[i] != NULL) {
        if ((strcmp(arglist[i] ,"|")) == 0) {
            has_pipe_sym = i;  /* i != 0 according to description */
            break;
        } 
        i++;
    }
    return has_pipe_sym;
}

/*
* A function that runs two child processes, with the output (stdout) of the 
* first process piped to the input (stdin) of the second process 
* helped here: 
*   -> recitation3&4 code files - papa_son_pipe2.c, wait_demo.c
*   -> https://stackoverflow.com/questions/3642732/using-dup2-for-piping
*/
int execute_pipig_two_processes(int count, char** arglist, int pipe_symbol) {
    int pipefd[2];
    pid_t pid1;
    pid_t pid2;
    int exit_code = -1;
    char** first_cmd = arglist;
    char** second_cmd = arglist + pipe_symbol + 1;
    arglist[pipe_symbol] = NULL;
    
    /* when error accures - my_pipe returns 0, my_fork returns -1 */
    if (!my_pipe(pipefd) || (pid1 = my_fork()) < 0) { return 0; }

    if (pid1 == 0) {
        /* first child */
        sigint_default_handler();  /* Foreground child terminate upon SIGINT */
        close(pipefd[0]);  /* close read side */
        my_dup2(pipefd, 1);
        close(pipefd[1]);
        my_execvp(first_cmd);
    } else {
        /* parent */
        if ((pid2 = my_fork()) < 0) { return 0; };  /* failed forking */

        if (pid2 == 0) {
            /* second child */
            sigint_default_handler(); /*Foreground child terminate upon SIGINT*/
            close(pipefd[1]);  /* close write side */
            my_dup2(pipefd, 0);
            close(pipefd[0]);
            my_execvp(second_cmd);
        } else {
            /* parent */
            close(pipefd[0]);
            close(pipefd[1]);

            /* wait for child processes */
            if ((waitpid(pid1, &exit_code, 0) == -1) && (errno != ECHILD)) {
                perror("Failed executing waitpid for first process in pipe");
                return 0;
            }
            if ((waitpid(pid2, &exit_code, 0) == -1) && (errno != ECHILD)) {
                perror("Failed executing waitpid for second process in pipe");
                return 0;
            }
        }
    }
    return 1;
}

/*
* A function that executes command so its standard output is redirected to the 
* output file
* helped here:
*   -> recitation4 code files - fifo_reader.c, fifo_writer.c, papa_son_pipe2.c
*/
int execute_redirect_stdout_to_file(int count, char** arglist) {
    char* file_name;
    pid_t pid;
    int fd;
    int exit_code = -1;

    char* file_name = arglist[count-1];
    arglist[count-2] = NULL;  /* ">" symbol change to NULL */
    
    if ((fd = open(arglist[file_name], O_RDWR | O_CREAT , 00766)) == -1) {
        perror("Failed executing open file");  /* failed opening file */
        return 0;
    }
    if ((pid = my_fork()) < 0) { return 0; };  /* failed forking */

    if (pid == 0) {
        /* child process */
        sigint_default_handler(); /*Foreground child terminate upon SIGINT*/
        my_dup2(fd, 1);  /* redirect output */
        my_execvp(arglist);
    } else {
        /* parent process - waits for child process */
        if ((waitpid(pid, &exit_code, 0) == -1) && (errno != ECHILD)) {
            perror("Failed executing waitpid for redirecting stdout to file");
            return 0;
        }
    }
    return 1;
}

/*
* A function to execute shell cmd without any symbol
* helped here:
*   -> recitation3 code files - exec_fork.c, 
*/
int execute_cmd_regulary(int count, char** arglist) {
    pid_t pid;
    int exit_code = -1;

    if ((pid = my_fork()) < 0) { return 0; };  /* failed forking */
    
    if (pid == 0) {
        /* child process */
        sigint_default_handler(); /*Foreground child terminate upon SIGINT*/
        my_execvp(count, arglist);
    } else {
        /* parent process - waits for child process */
        if ((waitpid(pid, &exit_code, 0) == -1) && (errno != ECHILD)) {
            perror("Failed executing waitpid for redirecting stdout to file");
            return 0;
        }
    }
    return 1;
}
