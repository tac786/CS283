#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dshlib.h"

/*
 * Function: print_dragon
 * ----------------------
 * Prints the Drexel Dragon ASCII art to the console.
 *
 * This function stores the ASCII dragon as a string with preserved
 * spacing and formatting, then prints it using printf().
 *
 * Usage:
 * - Called when the user enters the command "dragon" in the shell.
 * - Ensures correct alignment and formatting for display.
 *
 * No parameters.
 * No return value.
 */
 void print_dragon() {
     const char *dragon_ascii =
         "                                                                        @%%%%                       \n"
         "                                                                     %%%%%%                         \n"
         "                                                                    %%%%%%                          \n"
         "                                                                 % %%%%%%%           @              \n"
         "                                                                %%%%%%%%%%        %%%%%%%           \n"
         "                                       %%%%%%%  %%%%@         %%%%%%%%%%%%@    %%%%%%  @%%%%        \n"
         "                                  %%%%%%%%%%%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%%%%%%%%%%%%          \n"
         "                                %%%%%%%%%%%%%%%%%%%%%%%%%%   %%%%%%%%%%%% %%%%%%%%%%%%%%%           \n"
         "                               %%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%     %%%            \n"
         "                             %%%%%%%%%%%%%%%%%%%%%%%%%%%%@ @%%%%%%%%%%%%%%%%%%        %%            \n"
         "                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%                \n"
         "                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              \n"
         "                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%@%%%%%%@              \n"
         "      %%%%%%%%@           %%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%      %%                \n"
         "    %%%%%%%%%%%%%         %%@%%%%%%%%%%%%           %%%%%%%%%%% %%%%%%%%%%%%      @%                \n"
         "  %%%%%%%%%%   %%%        %%%%%%%%%%%%%%            %%%%%%%%%%%%%%%%%%%%%%%%                        \n"
         " %%%%%%%%%       %         %%%%%%%%%%%%%             %%%%%%%%%%%%@%%%%%%%%%%%                       \n"
         "%%%%%%%%%@                % %%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%                     \n"
         "%%%%%%%%@                 %%@%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  \n"
         "%%%%%%%@                   %%%%%%%%%%%%%%%           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              \n"
         "%%%%%%%%%%                  %%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      %%%%  \n"
         "%%%%%%%%%@                   @%%%%%%%%%%%%%%         %%%%%%%%%%%%@ %%%% %%%%%%%%%%%%%%%%%   %%%%%%%%\n"
         "%%%%%%%%%%                  %%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%% %%%%%%%%%\n"
         "%%%%%%%%%@%%@                %%%%%%%%%%%%%%%%@       %%%%%%%%%%%%%%     %%%%%%%%%%%%%%%%%%%%%%%%  %%\n"
         " %%%%%%%%%%                  % %%%%%%%%%%%%%%@        %%%%%%%%%%%%%%   %%%%%%%%%%%%%%%%%%%%%%%%%% %%\n"
         "  %%%%%%%%%%%%  @           %%%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  %%% \n"
         "   %%%%%%%%%%%%% %%  %  %@ %%%%%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%% \n"
         "    %%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%           @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%%%%%% \n"
         "     %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              %%%%%%%%%%%%%%%%%%%%%%%%%%%%        %%%   \n"
         "      @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  %%%%%%%%%%%%%%%%%%%%%%%%%               \n"
         "        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%%%%%%  %%%%%%%          \n"
         "           %%%%%%%%%%%%%%%%%%%%%%%%%%                           %%%%%%%%%%%%%%%  @%%%%%%%%%         \n"
         "              %%%%%%%%%%%%%%%%%%%%           @%@%                  @%%%%%%%%%%%%%%%%%%   %%%        \n"
         "                  %%%%%%%%%%%%%%%        %%%%%%%%%%                    %%%%%%%%%%%%%%%    %         \n"
         "                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%            \n"
         "                %%%%%%%%%%%%%%%%%%%%%%%%%%  %%%% %%%                      %%%%%%%%%%  %%%@          \n"
         "                     %%%%%%%%%%%%%%%%%%% %%%%%% %%                          %%%%%%%%%%%%%@          \n"
         "                                                                                 %%%%%%%@        \n";
 
     printf("%s\n", dragon_ascii);
 }
 
/*
 * This function implements the main loop for the dsh shell.
 * It continuously prompts the user for input using SH_PROMPT and 
 * processes commands accordingly.
 *
 * Functionality:
 * - Uses fgets() to read user input into cmd_buff.
 * - Handles EOF to allow headless testing.
 * - Removes the trailing newline character from cmd_buff.
 * - Checks if the user enters the "exit" command and terminates the shell.
 * - Checks if the user enters "dragon" and prints the ASCII art.
 * - Calls build_cmd_list() to parse the command line.
 * - Handles edge cases such as empty input and too many pipes.
 * - Displays parsed commands in the required format.
 *
 * Constants used from dshlib.h:
 * - SH_CMD_MAX              -> Maximum buffer size for user input.
 * - EXIT_CMD                -> Constant that terminates the dsh program.
 * - SH_PROMPT               -> The shell prompt.
 * - OK                      -> Command parsed properly.
 * - WARN_NO_CMDS            -> User command was empty.
 * - ERR_TOO_MANY_COMMANDS   -> Too many pipes were used.
 *
 * Expected Output:
 * - CMD_OK_HEADER           -> Displays parsed command details.
 * - CMD_WARN_NO_CMD         -> Prints a warning for an empty command.
 * - CMD_ERR_PIPE_LIMIT      -> Prints an error when too many pipes are used.
 *
 * Additional Feature:
 * - "dragon" command prints the Drexel Dragon ASCII art.
 *
 * See the provided test cases for further output expectations.
 */
 int main() {
     char cmd_buff[SH_CMD_MAX];  // buffer to store user input
     command_list_t clist;  // stores parsed commands
 
     while (1) {
         printf("%s", SH_PROMPT);  // display shell prompt
 
         // read user input
         if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
             printf("\n");
             break;  // exit loop on EOF
         }
 
         // remove the trailing newline character
         cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
 
         // check if the user entered "exit"
         if (strcmp(cmd_buff, EXIT_CMD) == 0) {
             exit(0);
         }
 
         // check if the user entered "dragon" and print ASCII art
         if (strcmp(cmd_buff, "dragon") == 0) {
             print_dragon();
             continue;  // skip further processing
         }
 
         // parse command line into command list
         int result = build_cmd_list(cmd_buff, &clist);
         if (result == WARN_NO_CMDS || result == ERR_TOO_MANY_COMMANDS) continue;  // skip printing total commands
 
         // print parsed command list
         printf("PARSED COMMAND LINE - TOTAL COMMANDS %d\n", clist.num);
         for (int i = 0; i < clist.num; i++) {
             printf("<%d> %s", i + 1, clist.commands[i].exe);
             if (clist.commands[i].args[0] != '\0') {
                 printf(" [%s]", clist.commands[i].args);
             }
             printf("\n");
         }
     }
 
     return 0;
 }