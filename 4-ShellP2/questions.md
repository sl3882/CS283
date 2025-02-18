1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  fork() is used to create a new child process, while execvp() replaces the memory space of the child process with a new program. fork does not interfere with the parent process. The parent can continue execution or monitor the status of the child, while the child process can execute a new program through execvp(). Without fork(), if you call execvp() directly, the memory of the parent process will be replaced, and it will not be able to continue.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**: If fork() fails, it returns -1 and sets errno to indicate the error. Your implementation must check for this error and handle it appropriately, typically by printing an error message and terminating the program.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: execvp() looks for commands in the directories listed in the PATH environment variable, which contains a colon-separated list of directories in which executable programs can be found. If the command is not found in any of these directories, execvp() fails and returns -1.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  wait() causes the parent process to wait for the child process to complete. If wait() is not called, the exit status of the child process will not be collected, resulting in a resource leak.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS(status) extracts the exit status of the child process from the status returned by wait(). This tells you how the child process exited, whether successfully or with an error.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  The build_cmd_buff() function handles quoted arguments by ensuring that any spaces within the quotes are included as part of a single argument. This is necessary because shell command parsing requires arguments with spaces (e.g. "Hello World") to be treated as a single token, not as multiple tokens separated by spaces. Proper handling of quoted arguments ensures that command line input is parsed correctly.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  In the previous assignment, the parsing logic did not handle quoted arguments properly. I refactored the code so that quoted strings are treated as a single argument and internal whitespace is preserved.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals on Linux systems are used to send asynchronous notifications to processes, typically to notify of events such as interrupts, illegal memory accesses, or termination requests. Unlike other forms of IPC such as message queues or shared memory, signals are simpler and do not require complex setup or synchronization. They are typically used for exception handling, process control, or communication with running processes.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGTERM (15): This is the default signal sent by the kill command. It politely asks a process to terminate. Most well-behaved programs will catch this signal and perform cleanup operations  before exiting.   

SIGKILL (9): This signal forces a process to terminate immediately. It cannot be caught or ignored. This is a last resort when a process is unresponsive or needs to be terminated urgently.   

SIGINT (2): This signal is typically generated when a user presses Ctrl+C. It's a request to interrupt the currently running process.

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  When a process receives SIGSTOP, it is suspended or stopped. This signal, unlike SIGINT, cannot be caught or ignored. It is a special signal used to suspend a process, and the process can only be resumed by receiving a SIGCONT signal.
