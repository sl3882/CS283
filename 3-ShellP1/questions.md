1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: fgets() reads in at most one less than size characters from stream and stores them into the buffer pointed to by s. Reading stops after an EOF or a newline. If a newline is read, it is stored into the buffer. A terminating null byte ('\0') is stored after the last character in the buffer.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Using fixed-size arrays can waste memory if the size is too large, or cause truncation and buffer overflow problems if the size is too small. Dynamic allocation can efficiently handle commands of various lengths.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming whitespace enables consistent command parsing and execution. Failure to trim whitespace may result in incorrect command interpretation and unexpected errors due to additional whitespace at the beginning or end.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:   1 .Output redirection: command > file.txt
                    2. Input redirection: command < file.txt
                    3.Error redirection: command 2> error.txt

                    Challenge: If the file already exists, we should overwrite it or save it separately so that an error message is provided. We should also ensure that errors are properly captured and the file is properly created or appended, depending on the situation.
                    

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection controls input and output, typically involving files (e.g., command > file.txt). Piping (|), on the other hand, connects the output of one command directly to the input of another (e.g., ls | grep txt). While redirection primarily deals with files, piping facilitates command chaining, allowing for more complex workflows by passing data between processes.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Separating STDERR and STDOUT prevents error messages from being mixed in with the regular output, allowing users to handle errors and output independently, such as logging errors separately or selectively suppressing output.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Our shell should basically capture STDERR messages separately from STDOUT and display them appropriately.