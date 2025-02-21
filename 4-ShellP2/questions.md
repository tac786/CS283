1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: The `fork` system call creates a new child process that runs as a copy of the parent. This allows the shell to remain active while the child process executes the new program via `execvp`. If we were to call `execvp` directly in the shell process, it would replace the shell itself, preventing further command execution. `fork` ensures the shell can continue accepting user commands even after executing another program.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**: If `fork()` fails, it returns `-1`, which means no child process was created. This can happen due to resource exhaustion or system limits. In our implementation, we check the return value of `fork()`, and if it fails, we print an error message using `perror("Fork failed")` and return an error code to indicate the failure. This ensures the shell does not crash unexpectedly.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: `execvp()` searches for the given command in the directories listed in the `PATH` environment variable. The `PATH` variable contains multiple directory paths separated by colons (`:`), and `execvp()` iterates through them to locate an executable file that matches the command name. If no match is found, it returns an error.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: The `wait()` system call makes the parent process pause execution until the child process completes. This prevents zombie processes from accumulating and ensures that system resources are properly released. If we didn’t call `wait()`, the child process would become a zombie process after termination, potentially leading to resource leaks and unnecessary process table entries.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: `WEXITSTATUS(status)` extracts the exit code from the child process’s termination status. This is important because it allows the shell to retrieve and display the exit status of executed commands, enabling users to check for success (`0`) or failure (non-zero values). It is also useful for scripting and error handling.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: My `build_cmd_buff()` function correctly identifies quoted arguments by tracking when a double quote (`"`) is encountered. When inside quotes, spaces are treated as part of the argument rather than as delimiters. This ensures that commands like `echo "hello world"` are properly parsed as a single argument (`["echo", "hello world"]`) instead of separate ones (`["echo", "hello", "world"]`). Without this, commands with spaces inside quotes would be misinterpreted.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: One major change was improving how whitespace and quoted arguments are handled to match standard shell behavior. The previous implementation did not fully support multiple spaces or properly parse quoted strings. A challenge during refactoring was ensuring that spaces inside quotes were preserved while extra spaces outside were ignored. This required additional logic to differentiate between normal spaces and quoted spaces.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals in Linux are used for asynchronous process control and communication. They allow one process (or the OS) to notify another process of an event, such as termination (`SIGTERM`) or an interrupt (`SIGINT`). Unlike other IPC mechanisms (such as pipes or shared memory), signals are simple notifications that do not carry complex data but instead indicate specific conditions that a process may need to handle.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  
    - **SIGKILL**: Immediately terminates a process and cannot be caught or ignored. Used when a process must be forcibly stopped.  
    - **SIGTERM**: Gracefully asks a process to terminate, allowing it to clean up resources before exiting. Used for standard process shutdowns.  
    - **SIGINT**: Sent when a user presses `Ctrl+C` in a terminal, interrupting the process. Often used to stop running programs interactively.  

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: `SIGSTOP` immediately suspends a process and cannot be caught, blocked, or ignored. This is different from `SIGINT`, which a process can handle or override. `SIGSTOP` is designed for debugging and process control, allowing a process to be paused and later resumed with `SIGCONT`. Since it is meant for system-wide process control, it is enforced at the kernel level and cannot be overridden.
