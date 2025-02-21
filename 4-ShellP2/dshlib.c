#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

int last_return_code = 0;  // stores the return code of the last command

// remove extra spaces at the start and end of a string
void trim_whitespace(char *str) {
    if (str == NULL) return;

    // move pointer forward to remove leading spaces
    while (isspace((unsigned char)*str)) str++;

    // find the end of the string and remove trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) *end-- = '\0';
}

// break down the input command into words (arguments)
int parse_input(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || strlen(cmd_line) == 0) return WARN_NO_CMDS;

    trim_whitespace(cmd_line);  // clean up spaces

    int argc = 0;
    bool in_quotes = false;
    char *arg_start = NULL, *cursor = cmd_line;

    // go through each character in the command
    while (*cursor) {
        if (*cursor == '"') {  // handle quoted arguments
            in_quotes = !in_quotes;
            *cursor = '\0';
            if (in_quotes) arg_start = cursor + 1;
        } 
        else if (!in_quotes && isspace(*cursor)) {  
            *cursor = '\0';  // mark end of an argument
            if (arg_start) cmd_buff->argv[argc++] = arg_start;
            arg_start = NULL;
        } 
        else {
            if (!arg_start) arg_start = cursor;  // start a new argument
        }
        cursor++;
    }

    // store the last argument if any
    if (arg_start) cmd_buff->argv[argc++] = arg_start;

    // null-terminate the list of arguments
    cmd_buff->argv[argc] = NULL;
    cmd_buff->argc = argc;
    return OK;
}

// check if the command is built-in (like cd, exit, rc, dragon)
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) return BI_NOT_BI;  // no command given

    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) exit(0);  // exit the shell

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc > 1) {
            if (chdir(cmd->argv[1]) != 0) {  // try changing directory
                perror("cd failed");
                last_return_code = ERR_EXEC_CMD;  // store error code
            } else {
                last_return_code = 0;  // success
            }
        }
        return BI_CMD_CD;
    }

    if (strcmp(cmd->argv[0], "dragon") == 0) {
        print_dragon();  // show ASCII dragon
        return BI_CMD_DRAGON;
    }

    if (strcmp(cmd->argv[0], "rc") == 0) {  // print last return code
        printf("%d\n", last_return_code);
        return BI_RC;
    }

    return BI_NOT_BI;  // command is not built-in
}

// show an error message if a command fails to run
void handle_exec_error(int errnum) {
    switch (errnum) {
        case ENOENT:
            fprintf(stderr, "command not found in PATH\n");
            break;
        case EACCES:
            fprintf(stderr, "permission denied: unable to execute command\n");
            break;
        default:
            fprintf(stderr, "execution error: %s\n", strerror(errnum));
            break;
    }
}

// run an external program (like ls, pwd, echo)
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();  // create a new process

    if (pid == 0) {  // child process
        if (execvp(cmd->argv[0], cmd->argv) == -1) {  // try running the command
            handle_exec_error(errno);
            exit(errno);  // exit with the error code
        }
    } 
    else if (pid > 0) {  // parent process
        int status;
        waitpid(pid, &status, 0);  // wait for the child process to finish
        last_return_code = WEXITSTATUS(status);  // save the return code
        return last_return_code;
    } 
    else {  // fork failed
        perror("fork failed");
        last_return_code = ERR_MEMORY;
        return ERR_MEMORY;
    }

    return last_return_code;
}

// the main shell loop: keep running until the user exits
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];  // store user input
    cmd_buff_t cmd;  // store parsed command

    while (1) {
        printf("%s", SH_PROMPT);  // show shell prompt

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {  // read user input
            printf("\n");
            break;  // exit loop if input is empty (Ctrl+D)
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';  // remove newline character

        int parse_result = parse_input(cmd_line, &cmd);
        if (parse_result == WARN_NO_CMDS) {
            printf("%s\n", CMD_WARN_NO_CMD);  // show warning for empty input
            continue;
        }

        // if it's a built-in command, run it, otherwise execute normally
        if (exec_built_in_cmd(&cmd) == BI_NOT_BI) exec_cmd(&cmd);
    }

    return OK;
}