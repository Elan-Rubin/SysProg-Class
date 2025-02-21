1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  the value of the fork function is that it creates a whole new child process. it's important that we still preserve the parent process. the execvp replaces the current process with the new one, so without fork we'd lose the shell process

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  if the fork syscall fails, then it'll return -1 to the parent function. for this reason, we check the return value of the fork and make sure to continue running instead of crashing.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp uses the environment variable called PATH to figure out which command to execute. this would work by replacing the current process with a new one that takes place at the specified path.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  the wait function makes sure that we won't have any zombie processes. this is part of the parent process and it waits until completion so that we are sure of the child's exit status/return value

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: the WEXITSTATUS function extracts the exit status code value from the wait() function, because wait usually returns other information but we really just need the exit code

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  the handling of quoted arguments preserves literal interpretation of special characters and preserve the spaces within quoted strings. we also handle them by trimming eading and trailing spaces

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  the main parsing changes are removing the pipe handling for now, and switching the structure form command list to cmd buffer. we also adapted the parsing of quoted strings

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  even though signals arent implemented in this assignment, they will let us do some future shell features like handling ctrl+c and this will complement 

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**: these signals are used for the following. SIGKILL: forcefully terminates processes, SIGTERM: termination request, SIGINT: interrupt handling

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP suspends the execution and then we can make it resume with the SIGCONT signal
