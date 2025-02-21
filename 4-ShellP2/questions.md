### 1. Why do we use `fork/execvp` instead of just calling `execvp` directly? What value does `fork` provide?
> **Answer**:  
> We use `fork()` to create a separate child process before calling `execvp()`. This allows the parent process, which in this case is our dsh shell, to keep running while the child process executes the new program. If we called `execvp()` directly, it would replace our shell process, meaning we wouldn’t be able to take any more commands after running a program.  
>  
> By using `fork()`, our shell can continue accepting user input while handling external commands in separate processes.

### 2. What happens if the `fork()` system call fails? How does your implementation handle this scenario?
> **Answer**:  
> If `fork()` fails, it returns `-1`, which means the system couldn’t create a new process. This usually happens when there are too many processes running, or the system is out of resources.  
>  
> In our implementation, if `fork()` fails, we print an error message using `perror("Fork failed")`, and we return an error code (`ERR_MEMORY`). This ensures that the user knows something went wrong instead of the shell crashing silently.

### 3. How does `execvp()` find the command to execute? What system environment variable plays a role in this process?
> **Answer**:  
> `execvp()` searches for the command in the directories listed in the `PATH` environment variable. The `PATH` variable is a colon-separated list of directories where executable files are stored.  
>  
> For example, when we type `ls`, `execvp()` looks for `/bin/ls`, `/usr/bin/ls`, and so on until it finds the right executable or fails with an error like "command not found in PATH."

### 4. What is the purpose of calling `wait()` in the parent process after forking? What would happen if we didn’t call it?
> **Answer**:  
> `wait()` ensures that the parent process (our shell) waits for the child process to finish before continuing. This prevents zombie processes, which are child processes that have finished execution but still exist in the system’s process table.  
>  
> If we didn’t call `wait()`, the shell would immediately move on to the next command, leaving unfinished child processes hanging around, which could eventually slow down or crash the system.

### 5. In the referenced demo code, we used `WEXITSTATUS()`. What information does this provide, and why is it important?
> **Answer**:  
> `WEXITSTATUS(status)` extracts the exit code of the child process. This is important because it lets our shell know if a command ran successfully (`0` means success) or if there was an error (any non-zero value).  
>  
> In our implementation, we store this value in `last_return_code`, so the user can check it using the `rc` command.

### 6. Describe how your implementation of `build_cmd_buff()` handles quoted arguments. Why is this necessary?
> **Answer**:  
> Our `build_cmd_buff()` function correctly handles quoted arguments by treating text inside double quotes as a single argument. Without this, commands like:
> ```sh
> echo "hello    world"
> ```
> would incorrectly split into `["echo", "hello", "world"]` instead of preserving the spaces as `["echo", "hello    world"]`.  
>  
> This is necessary to ensure that user input is processed the same way as in real shells.

### 7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?
> **Answer**:  
> The biggest change was improving how we handle multiple spaces and quoted arguments. In the previous assignment, we had trouble with cases like:
> ```sh
> echo   hello   world
> ```
> where extra spaces weren’t handled correctly. Now, we collapse extra spaces **outside** of quotes while preserving spaces **inside** quotes.  
>  
> One challenge was making sure that quoted text wasn’t split incorrectly. We had to carefully toggle a flag (`in_quotes`) to track when we were inside quotes.

---

## Linux Signals Questions

### 8. What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?
> **Answer**:  
> Signals are a way for processes to communicate with each other or receive notifications from the operating system. They are **asynchronous**, meaning they can be sent at any time, unlike other IPC methods (pipes, message queues) that require explicit sending and receiving.  
>  
> For example, pressing `Ctrl+C` sends a `SIGINT` signal to a process, telling it to terminate.

### 9. Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?
> **Answer**:  
> - **SIGKILL (9)** – Forces a process to stop immediately. Cannot be ignored or caught. Used when a process is unresponsive.  
> - **SIGTERM (15)** – Politely asks a process to terminate. The process can handle this signal and perform cleanup before exiting.  
> - **SIGINT (2)** – Sent when the user presses `Ctrl+C`. Typically used to stop a running process interactively.

### 10. What happens when a process receives `SIGSTOP`? Can it be caught or ignored like `SIGINT`? Why or why not?
> **Answer**:  
> `SIGSTOP` pauses a process, making it unable to execute until it receives a `SIGCONT` signal. Unlike `SIGINT`, `SIGSTOP` **cannot be caught or ignored** because it's a system-level signal designed to ensure the process is actually stopped.  
>  
> This is useful for debugging (`SIGSTOP` pauses a process so you can inspect it) and for job control in shells (e.g., `Ctrl+Z` sends `SIGSTOP`).