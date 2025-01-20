#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BUFFER_SZ 50

// Function prototypes
void usage(char *);                                 // Displays the usage instructions for the program
void print_buff(char *, int);                       // Prints the contents of the buffer
int setup_buff(char *, char *, int);                // Sets up the buffer with the input string and padding
int count_words(char *, int, int);                  // Counts the number of words in the buffer
void reverse_string(char *, int);                   // Reverses the string in the buffer
void print_words_with_length(char *, int, int);     // Prints each word and its length
int replace_substring(char *, char *, char *, int); // Replaces a substring in the buffer with another substring

// Setup buffer with padding
int setup_buff(char *buff, char *user_str, int len) {
    int str_len = 0;
    char *src = user_str;
    char *dest = buff;
    bool at_space = true; // Tracks consecutive spaces

    // Process the input string and copy into the buffer
    while (*src && str_len < len) {
        if (*src == ' ') {
            if (!at_space) { // Only copy a single space
                *dest++ = ' ';
                str_len++;
            }
            at_space = true;
        } else {
            *dest++ = *src; // Copy non-space character
            str_len++;
            at_space = false;
        }
        src++;
    }

    // Handle trailing spaces
    if (str_len > 0 && *(dest - 1) == ' ') {
        dest--;
        str_len--;
    }

    // Check if the input string exceeds the buffer size
    if (*src) {
        fprintf(stderr, "Error: Input string exceeds buffer size.\n");
        return -1;
    }

    // Pad the remaining buffer with '.'
    while (str_len < len) {
        *dest++ = '.';
        str_len++;
    }

    return str_len;
}

// Print the buffer
void print_buff(char *buff, int len) {
    printf("Buffer:  [");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i)); // Print each character in the buffer
    }
    printf("]\n");
}

// Display usage instructions
void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

// Count words in the buffer
int count_words(char *buff, int len, int str_len) {
    (void)len; // Suppress unused parameter warning
    bool at_start = true;
    int word_count = 0;

    for (int i = 0; i < str_len; i++) {
        char c = *(buff + i); // Get the current character
        if (c == ' ') {
            at_start = true; // Set start flag to true when encountering a space
        } else if (at_start) {
            word_count++;
            at_start = false; // Set start flag to false
        }
    }
    return word_count;
}

// Reverse the string
void reverse_string(char *buff, int str_len) {
    char *start = buff;
    char *end = buff + str_len - 1;

    // Reverse only the valid string content (ignoring the dots at first)
    while (start < end) {
        char temp = *start;
        *start = *end;  // Swap the characters
        *end = temp;    // Assign the held character to the opposite end
        start++;
        end--;
    }

    // Move all dots to the end of the buffer
    char temp_buffer[BUFFER_SZ];
    int temp_index = 0;

    // Copy non-dot characters into temp_buffer
    for (int i = 0; i < str_len; i++) {
        if (buff[i] != '.') {
            temp_buffer[temp_index++] = buff[i];
        }
    }

    // Fill the rest of temp_buffer with dots
    while (temp_index < BUFFER_SZ) {
        temp_buffer[temp_index++] = '.';
    }

    // Copy back to the original buffer
    for (int i = 0; i < BUFFER_SZ; i++) {
        buff[i] = temp_buffer[i];
    }
}

// Print words with their lengths
void print_words_with_length(char *buff, int len, int str_len) {
    (void)len; // Suppress unused parameter warning
    bool at_start = true;
    int char_count = 0;
    int word_count = 0;

    printf("Word Print\n----------\n");
    for (int i = 0; i < str_len; i++) {
        char c = *(buff + i);
        if (c == ' ') {
            if (!at_start) {
                printf(" (%d)\n", char_count); // Print length of the word and move to next line
                char_count = 0;              // Reset the character counter
                word_count++;
            }
            at_start = true;
        } else if (c != '.') { // Ignore padding dots
            if (at_start) {
                at_start = false;
                printf("%d. %c", word_count + 1, c); // Print word number and first character
            } else {
                printf("%c", c); // Print subsequent characters of the word
            }
            char_count++;
        }
    }

    if (char_count > 0) {
        printf(" (%d)\n", char_count); // Print the length of the last word
        word_count++;
    }
}

// Replace a substring with another substring
int replace_substring(char *buff, char *old_sub, char *new_sub, int len) {
    (void)len; // Suppress unused parameter warning
    char temp[BUFFER_SZ];
    char *src = buff;
    char *dest = temp;
    char *pos = NULL;

    // Find the first occurrence of old_sub in buff
    while (*src) {
        if (strncmp(src, old_sub, strlen(old_sub)) == 0) {
            pos = src;
            break;
        }
        src++;
    }

    if (!pos) {
        return -1; // Substring not found
    }

    // Copy characters before the old substring
    src = buff;
    while (src < pos && dest < temp + BUFFER_SZ - 1) {
        *dest++ = *src++;
    }

    // Copy the new substring
    char *new_sub_ptr = new_sub;
    while (*new_sub_ptr && dest < temp + BUFFER_SZ - 1) {
        *dest++ = *new_sub_ptr++;
    }

    // Copy characters after the old substring
    src += strlen(old_sub);
    while (*src && dest < temp + BUFFER_SZ - 1) {
        *dest++ = *src++;
    }

    // Null-terminate the resulting string
    *dest = '\0';

    // Check for truncation
    if (dest - temp >= BUFFER_SZ) {
        fprintf(stderr, "Warning: Replacement caused truncation of the buffer.\n");
        return -2;
    }

    // Copy temp back to buff
    char *temp_ptr = temp;
    char *buff_ptr = buff;
    while (*temp_ptr && buff_ptr < buff + BUFFER_SZ - 1) {
        *buff_ptr++ = *temp_ptr++;
    }

    // Null-terminate buff
    *buff_ptr = '\0';

    return 0;
}

int main(int argc, char *argv[]) {
    char *buff;             // Placeholder for the internal buffer
    char *input_string;     // Holds the string provided by the user on cmd line
    char opt;               // Used to capture user option from cmd line
    int rc;                 // Used for return codes
    int user_str_len;       // Length of user supplied string

    // TODO: #1. WHY IS THIS SAFE, aka what if argv[1] does not exist?
    // This condition ensures safety:
    // - (argc < 2) checks that the user provided at least 2 arguments. If not,
    //   accessing argv[1] would cause undefined behavior.
    // - (*argv[1] != '-') ensures that the first character of argv[1] is a valid option flag.
    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1);   // Get the option flag

    // Handle the help flag and then exit normally
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    // TODO: #2 Document the purpose of the if statement below
    // This condition checks if the user provided a string argument (argv[2]) alongside the option flag.
    // The program requires at least 3 arguments: 
    // - argv[0]: The program name
    // - argv[1]: The option flag
    // - argv[2]: The input string
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2];

    // TODO: #3 Allocate space for the buffer using malloc and
    //       handle error if malloc fails by exiting with a return code of 99
    buff = (char *)malloc(BUFFER_SZ * sizeof(char));
    if (buff == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for buffer.\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ); // Setup the buffer
    if (user_str_len < 0) {
        fprintf(stderr, "Error setting up buffer, error = %d\n", user_str_len);
        free(buff); // Free allocated memory before exiting
        exit(2);
    }

    switch (opt) {
        case 'c': // Handle word count
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                fprintf(stderr, "Error counting words, rc = %d\n", rc);
                free(buff); // Free allocated memory before exiting
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            print_buff(buff, BUFFER_SZ); // Print buffer here
            break;

        case 'r': // Reverse the string
            reverse_string(buff, user_str_len);
            print_buff(buff, BUFFER_SZ); // Print buffer here
            break;

        case 'w': // Print words and their lengths
            print_words_with_length(buff, BUFFER_SZ, user_str_len); // Prints buffer internally
            break;

        case 'x': {
            if (argc < 5) {
                fprintf(stderr, "Error: Replace option requires two additional arguments.\n");
                free(buff);
                exit(1);
            }
            char *old_sub = argv[3];
            char *new_sub = argv[4];
            int result = replace_substring(buff, old_sub, new_sub, BUFFER_SZ);
            if (result == -1) {
                fprintf(stderr, "Error: Substring '%s' not found.\n", old_sub);
                free(buff);
                exit(2);
            } else if (result == -2) {
                fprintf(stderr, "Warning: Replacement caused truncation of the buffer.\n");
            }
            print_buff(buff, BUFFER_SZ); // Print buffer here
            break;
        }
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    free(buff); // Free allocated memory
    exit(0);
}

// TODO: #7 Notice all of the helper functions provided in the starter take both the buffer as well as the length.
// Why do you think providing both the pointer and the length is a good practice?
// Answer:
// - Providing both the buffer pointer and its length ensures that functions can handle the buffer safely, even if
//   the actual buffer size is larger than the required size for a specific operation.
// - This avoids potential buffer overflows and undefined behavior, providing a clear contract for the function's usage.
