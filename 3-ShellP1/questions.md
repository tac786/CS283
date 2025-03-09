1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: `fgets()` is a good choice because it reads an entire line of input safely, preventing buffer overflows. It also stops reading when it encounters a newline (`\n`) or reaches the maximum buffer size. This allows us to handle user input properly and detect **end-of-file (EOF)** for cases when the shell runs in headless mode.

2. You needed to use `malloc()` to allocate memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**: `malloc()` allows us to dynamically allocate memory for `cmd_buff`, making the shell more flexible. Instead of using a fixed-size array that might waste memory or be too small, dynamic allocation lets us adjust the buffer size as needed. This also helps prevent stack overflow in case of very large inputs.

3. In `dshlib.c`, the function `build_cmd_list()` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming leading and trailing spaces ensures that commands are correctly parsed. If we don't trim spaces, empty commands (`""`) or incorrect arguments might be stored, leading to execution errors. For example, `"  ls   "` without trimming might be stored as `"  ls"` with extra spaces, causing command mismatches or unnecessary errors.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  
    > 1. **Redirect output to a file:**  
    >    ```bash
    >    ls > output.txt
    >    ```
    >    **Challenge:** We need to properly use `dup2()` to redirect STDOUT to a file descriptor.  
    >
    > 2. **Redirect input from a file:**  
    >    ```bash
    >    sort < data.txt
    >    ```
    >    **Challenge:** Our shell must open the file, replace STDIN with it, and ensure proper error handling if the file does not exist.  
    >
    > 3. **Redirect both STDOUT and STDERR:**  
    >    ```bash
    >    gcc program.c > output.log 2>&1
    >    ```
    >    **Challenge:** We must redirect both streams correctly and maintain output order without mixing data improperly.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  
    > - **Redirection (`>`, `<`)** sends output or input to a file instead of the terminal.  
    > - **Piping (`|`)** connects the output of one command as the input to another.  
    > - **Example:**  
    >   - **Redirection:** `ls > files.txt` (saves output to a file).  
    >   - **Piping:** `ls | grep ".c"` (filters output before displaying).  
    > - **Key difference:** Redirection deals with files, while piping connects processes.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  
    > STDERR is used for errors, while STDOUT is for normal output. Keeping them separate helps in debugging. If both were mixed, it would be harder to filter errors from valid output. For example, in `ls nonexistentfile 2> error.log`, we can save errors without affecting the normal output.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  
    > - Our shell should **check return codes** and print error messages if commands fail.  
    > - If a command fails, STDERR should be displayed to inform the user.  
    > - We should allow users to merge STDOUT and STDERR using `2>&1` for cases where they want a combined output.  
    > - To implement merging, we need to properly use `dup2()` to redirect STDERR to the same file descriptor as STDOUT.