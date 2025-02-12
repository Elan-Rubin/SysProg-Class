1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: fgets reads a line of input from the standard input channel, which makes it so that the user can type into the terminal/shell. The fgets command will stop at a newline or EOF so this makes it nice and easy to use, and the user can just hit enter to apply the command.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  I think that either would work actually, but we ended up using malloc() for the cmd_buff because of the dynamic sized allocation in the parsing of the command string.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming is necessary when you're doing shell commands, because the user should have a bit of leeway/forgiveness when they're typing, and it should still recognize when there are characters that need to be trimmed away.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  there is output redirection (>) where the challenge might be handling creation of new files. Next would be input redirection (<) where we'd have to verify if the file actually is there. Last is error redirection (2>) where we'd need to keep the streams separated.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection connects commands with files and piping connects two commands. Redirection can work with only one command, but piping is meant for multiple.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**: Separating the output streams would be important so that we can see the errors even if we don't want to see what's usually logged. This is because errors are more important to be visible, generally

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  For error handling, our shell should print the stderr in a different color, and maybe even use the stderr separation I mentioned in the previous question