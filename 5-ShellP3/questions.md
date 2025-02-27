1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

In the implementation, waitpid() is used to cause the parent shell to wait for all child processes to complete execution before continuing to accept input. If waitpid() is not called, the child processes may terminate without the parent shell collecting their exit status, resulting in "zombie" processes that remain in the process table and consume system resources.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

After redirecting input or output through a pipe by calling dup2(), you should close any unused pipe ends to avoid data leaking through the channel unintentionally and to avoid file descriptor exhaustion. Leaving pipe ends open can lead to unintended behavior, such as incomplete data transfers or deadlocks where processes wait indefinitely for data that never arrives because the pipe is still open and waiting for more input.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command is implemented as a builtin because it changes the current working directory of the shell process itself. External commands are executed in a child process, so any changes to the working directory only affect the child process, not the shell.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

To handle an arbitrary number of pipe commands, the shell implementation must dynamically allocate memory to handle the increasing number of commands. It must also store the input and output of each command, either using a dynamic data structure such as a linked list or reallocating memory as needed. These tradeoffs include increased memory management complexity, potential performance overhead due to frequent reallocation, and the risk of memory leaks or fragmentation if not handled properly.
