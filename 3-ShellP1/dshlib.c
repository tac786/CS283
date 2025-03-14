#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 * Function: build_cmd_list
 * ------------------------
 * Parses a command line string and populates the command_list_t structure
 *
 * Parameters:
 * - cmd_line:  A string containing the user's input command
 * - clist:     A pointer to the command_list_t structure
 *
 * Functionality:
 * - Splits the command string by pipe ('|') characters to separate commands
 * - Trims leading and trailing spaces from each command
 * - Extracts the first token as the executable name and the rest as arguments
 * - Stores parsed commands and arguments in clist->commands
 * - Limits the number of piped commands to CMD_MAX
 * - Handles edge cases such as empty input, excessive pipes, and missing arguments
 *
 * Error Handling:
 * - WARN_NO_CMDS:              Printed and returned when the user input is empty
 * - ERR_TOO_MANY_COMMANDS:     Printed and returned when the number of piped commands exceeds CMD_MAX
 * - ERR_CMD_OR_ARGS_TOO_BIG:   Returned if a command name or argument string is too long
 *
 * Expected Behavior:
 * - If the input is empty, it prints CMD_WARN_NO_CMD
 * - If the number of piped commands exceeds CMD_MAX, it prints CMD_ERR_PIPE_LIMIT and exits early
 * - Properly formats and stores commands and arguments in the command_list_t structure
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (cmd_line == NULL || clist == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    clist->num = 0;  // initialize command count

    char *cmd_start = cmd_line;
    char *cmd_end;
    int command_count = 0;  // count commands before parsing

    // first pass: count commands
    char *temp_ptr = cmd_line;
    while ((temp_ptr = strchr(temp_ptr, PIPE_CHAR))) {
        command_count++;
        temp_ptr++;  // move past the pipe character
    }
    command_count++;  // count the last command (no trailing pipe)

    // if there are no commands (empty input)
    if (command_count == 1 && strlen(cmd_line) == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    // if the command count exceeds CMD_MAX, print an error and return immediately
    if (command_count > CMD_MAX) {
        printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        return ERR_TOO_MANY_COMMANDS;  // exit before parsing
    }

    // parsing commands
    while ((cmd_end = strchr(cmd_start, PIPE_CHAR)) || *cmd_start != '\0') {
        if (cmd_end) {
            *cmd_end = '\0';  // replace | with NULL to split the command
        }

        // trim leading and trailing spaces
        while (*cmd_start == SPACE_CHAR) cmd_start++;
        char *end = cmd_start + strlen(cmd_start) - 1;
        while (end > cmd_start && *end == SPACE_CHAR) end--;
        *(end + 1) = '\0';

        if (*cmd_start == '\0') {
            // skip empty commands caused by consecutive pipes (e.g., "cmd1 || cmd2")
            cmd_start = cmd_end + 1;
            continue;
        }

        // extract executable
        char *exe_token = strtok(cmd_start, " ");
        if (exe_token == NULL) {
            printf(CMD_WARN_NO_CMD);
            return WARN_NO_CMDS;
        }

        strncpy(clist->commands[clist->num].exe, exe_token, EXE_MAX - 1);
        clist->commands[clist->num].exe[EXE_MAX - 1] = '\0';  // ensure null-termination

        // extract arguments (everything after the first token)
        char *args = strtok(NULL, "");  // get remaining string as args
        if (args == NULL) {
            clist->commands[clist->num].args[0] = '\0';  // no arguments
        } else {
            if (strlen(args) >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;  // arguments too long
            }
            strncpy(clist->commands[clist->num].args, args, ARG_MAX - 1);
            clist->commands[clist->num].args[ARG_MAX - 1] = '\0';
        }

        clist->num++;  // move to the next command
        if (!cmd_end) break;  // exit loop if there's no more pipes

        cmd_start = cmd_end + 1;  // move to the next command after the pipe
    }

    return OK;  // successfully parsed commands
}
