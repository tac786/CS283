#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

// global variable to store the return code of the last executed command
int last_return_code = 0;

/* trim_whitespace
 * removes leading and trailing whitespace from the provided string
 */
void trim_whitespace(char *str) {
    if (!str) return;
    // remove leading whitespace
    char *start = str;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != str)
        memmove(str, start, strlen(start) + 1);
    // remove trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// helper function to set up redirection for a command (handles infile, outfile, and append mode)
static void setup_redirection(cmd_buff_t *cmd) {
    if (cmd->infile) {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd < 0) { perror("open infile"); exit(1); }
        if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 infile"); close(fd); exit(1); }
        close(fd);
    }
    if (cmd->outfile) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        int fd = open(cmd->outfile, flags, 0666);
        if (fd < 0) { perror("open outfile"); exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 outfile"); close(fd); exit(1); }
        close(fd);
    }
}

/* exec_built_in_cmd
 * checks and executes built-in commands (exit, cd, dragon)
 * if the command is not built-in, returns BI_NOT_BI
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0)
        return BI_NOT_BI;
    // handle exit command: print exiting... and return BI_CMD_EXIT
    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) { 
        printf("exiting...\n"); 
        return BI_CMD_EXIT; 
    }
    // handle cd command
    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc > 1) {
            if (chdir(cmd->argv[1]) != 0) { 
                perror("cd failed"); 
                last_return_code = ERR_EXEC_CMD; 
            } else {
                last_return_code = 0;
            }
        }
        return BI_CMD_CD;
    }
    // handle dragon command to print ascii art
    if (strcmp(cmd->argv[0], "dragon") == 0) { 
        print_dragon(); 
        return BI_CMD_DRAGON; 
    }
    return BI_NOT_BI;
}

/* exec_cmd
 * executes a single external command by forking a child process,
 * sets up any redirection, and then calls execvp in the child.
 */
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) { 
        perror("fork failed"); 
        last_return_code = ERR_MEMORY; 
        return ERR_MEMORY; 
    }
    if (pid == 0) {
        // in child: set up redirection and execute the command
        setup_redirection(cmd);
        if (execvp(cmd->argv[0], cmd->argv) == -1) { 
            perror("execvp"); 
            exit(errno); 
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        last_return_code = WEXITSTATUS(status);
        return last_return_code;
    }
    return last_return_code;
}

/* execute_pipeline
 * sets up pipes between commands in a pipeline and forks a child for each command.
 * each child applies pipe redirection then any file redirection before executing the command.
 */
int execute_pipeline(command_list_t *clist) {
    int num = clist->num;
    if (num < 1)
        return WARN_NO_CMDS;
    if (num > CMD_MAX) { 
        fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX); 
        fprintf(stderr, "\n"); 
        return ERR_TOO_MANY_COMMANDS; 
    }
    int pipes[CMD_MAX - 1][2];
    // create necessary pipes
    for (int i = 0; i < num - 1; i++) { 
        if (pipe(pipes[i]) < 0) { 
            perror("pipe"); 
            return ERR_MEMORY; 
        } 
    }
    pid_t pids[CMD_MAX];
    for (int i = 0; i < num; i++) {
        pid_t pid = fork();
        if (pid < 0) { 
            perror("fork"); 
            return ERR_MEMORY; 
        }
        if (pid == 0) {
            // if not the first command, use the previous pipe as stdin
            if (i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) < 0) { 
                perror("dup2"); 
                exit(errno); 
            }
            // if not the last command, use the current pipe as stdout
            if (i < num - 1 && dup2(pipes[i][1], STDOUT_FILENO) < 0) { 
                perror("dup2"); 
                exit(errno); 
            }
            // close all pipes in the child
            for (int j = 0; j < num - 1; j++) { 
                close(pipes[j][0]); 
                close(pipes[j][1]); 
            }
            // set up any file redirection for this command
            setup_redirection(&clist->commands[i]);
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1) { 
                perror("execvp"); 
                exit(errno); 
            }
        } else { 
            pids[i] = pid; 
        }
    }
    // parent closes all pipe fds
    for (int i = 0; i < num - 1; i++) { 
        close(pipes[i][0]); 
        close(pipes[i][1]); 
    }
    int status;
    // wait for all child processes to finish
    for (int i = 0; i < num; i++) { 
        waitpid(pids[i], &status, 0); 
    }
    return OK;
}

/* build_cmd_buff
 * parses a single command line (no pipes) into a command buffer,
 * tokenizes arguments, and detects redirection symbols (<, >, >>)
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || strlen(cmd_line) == 0)
        return WARN_NO_CMDS;
    // initialize redirection fields
    cmd_buff->infile = NULL; 
    cmd_buff->outfile = NULL; 
    cmd_buff->append = false;
    // duplicate the command string so we can modify it
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer)
        return ERR_MEMORY;
    int argc = 0; 
    bool in_quotes = false; 
    char *arg_start = NULL;
    // basic tokenization by space and quotes
    for (char *cursor = cmd_buff->_cmd_buffer; *cursor; cursor++) {
        if (*cursor == '"') { 
            in_quotes = !in_quotes; 
            *cursor = '\0'; 
            if (in_quotes) 
                arg_start = cursor + 1; 
        } else if (!in_quotes && isspace((unsigned char)*cursor)) { 
            *cursor = '\0'; 
            if (arg_start) { 
                cmd_buff->argv[argc++] = arg_start; 
                arg_start = NULL; 
            } 
        } else { 
            if (!arg_start) 
                arg_start = cursor; 
        }
    }
    if (arg_start)
        cmd_buff->argv[argc++] = arg_start;
    cmd_buff->argv[argc] = NULL; 
    cmd_buff->argc = argc;
    // parse for redirection symbols and remove them from the argument list
    int i = 0;
    while (i < argc) {
        if (strcmp(cmd_buff->argv[i], "<") == 0) {
            if (i + 1 < argc) { 
                cmd_buff->infile = cmd_buff->argv[i + 1]; 
                for (int j = i; j < argc - 2; j++) 
                    cmd_buff->argv[j] = cmd_buff->argv[j + 2]; 
                argc -= 2; 
            } else break;
        } else if (strcmp(cmd_buff->argv[i], ">>") == 0) {
            if (i + 1 < argc) { 
                cmd_buff->outfile = cmd_buff->argv[i + 1]; 
                cmd_buff->append = true; 
                for (int j = i; j < argc - 2; j++) 
                    cmd_buff->argv[j] = cmd_buff->argv[j + 2]; 
                argc -= 2; 
            } else break;
        } else if (strcmp(cmd_buff->argv[i], ">") == 0) {
            if (i + 1 < argc) { 
                cmd_buff->outfile = cmd_buff->argv[i + 1]; 
                cmd_buff->append = false; 
                for (int j = i; j < argc - 2; j++) 
                    cmd_buff->argv[j] = cmd_buff->argv[j + 2]; 
                argc -= 2; 
            } else break;
        } else i++;
    }
    cmd_buff->argv[argc] = NULL; 
    cmd_buff->argc = argc;
    return OK;
}

/* free_cmd_buff
 * frees the duplicated command string and resets the command buffer fields
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) { 
        free(cmd_buff->_cmd_buffer); 
        cmd_buff->_cmd_buffer = NULL; 
    }
    cmd_buff->argc = 0; 
    cmd_buff->infile = NULL; 
    cmd_buff->outfile = NULL; 
    cmd_buff->append = false;
    return OK;
}

/* build_cmd_list
 * splits a pipeline command line on the '|' character into separate commands,
 * and builds a command buffer for each sub-command
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || strlen(cmd_line) == 0)
        return WARN_NO_CMDS;
    clist->num = 0;
    char *saveptr, *token = strtok_r(cmd_line, PIPE_STRING, &saveptr);
    while (token) {
        // trim leading whitespace
        while (isspace((unsigned char)*token)) token++;
        // trim trailing whitespace
        char *end = token + strlen(token) - 1;
        while (end > token && isspace((unsigned char)*end)) { *end = '\0'; end--; }
        if (strlen(token) > 0) {
            if (clist->num >= CMD_MAX)
                return ERR_TOO_MANY_COMMANDS;
            int rc = build_cmd_buff(token, &clist->commands[clist->num]);
            if (rc != OK)
                return rc;
            clist->num++;
        }
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    if (clist->num == 0)
        return WARN_NO_CMDS;
    return OK;
}

/* free_cmd_list
 * frees the command buffers for each command in the list
 */
int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++)
        free_cmd_buff(&cmd_lst->commands[i]);
    cmd_lst->num = 0;
    return OK;
}

/* exec_local_cmd_loop
 * main shell loop: prints the prompt, reads user input, builds command buffers,
 * and executes either a single command or a pipeline. breaks out on exit.
 */
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;
    cmd_buff_t cmd;
    while (1) {
        printf("%s", SH_PROMPT);
        if (!fgets(cmd_line, SH_CMD_MAX, stdin)) { 
            printf("\n"); 
            break; 
        }
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        if (strlen(cmd_line) == 0) { 
            printf("%s\n", CMD_WARN_NO_CMD); 
            continue; 
        }
        // if there's a pipe, process a pipeline command
        if (strchr(cmd_line, PIPE_CHAR)) {
            int rc = build_cmd_list(cmd_line, &clist);
            if (rc == WARN_NO_CMDS) { 
                printf("%s\n", CMD_WARN_NO_CMD); 
                continue; 
            } else if (rc == ERR_TOO_MANY_COMMANDS) { 
                fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX); 
                fprintf(stderr, "\n"); 
                continue; 
            } else if (rc != OK) { 
                fprintf(stderr, "error building command list\n"); 
                continue; 
            }
            rc = execute_pipeline(&clist);
            if (rc != OK) 
                fprintf(stderr, "pipeline execution failed\n");
            free_cmd_list(&clist);
        } else {
            // process a single command
            int rc = build_cmd_buff(cmd_line, &cmd);
            if (rc == WARN_NO_CMDS) { 
                printf("%s\n", CMD_WARN_NO_CMD); 
                continue; 
            } else if (rc != OK) { 
                fprintf(stderr, "error building command buffer\n"); 
                continue; 
            }
            Built_In_Cmds bi = exec_built_in_cmd(&cmd);
            if (bi == BI_CMD_EXIT) { 
                free_cmd_buff(&cmd); 
                break; 
            }
            if (bi == BI_NOT_BI) { 
                rc = exec_cmd(&cmd); 
                if (rc == ERR_EXEC_CMD) 
                    fprintf(stderr, "command execution failed\n"); 
            }
            free_cmd_buff(&cmd);
        }
    }
    return OK;
}