1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

We use waitpid() in a loop to wait for each of the child process to finish befre continuing. If this wasn't part of the code, then it would be possible for the child processes to become zombies/detached, and then they'd just waste resources. This could make it unpredictable and slow.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

If you don't close the unused ends of a pipe after using dup2(), you might run into some annoying issues. For one, you could end up with file descriptor leaks, which are basically like leaving a bunch of doors open in your house. Also, if you leave the write end of a pipe open, the reading process might never get the signal that there's no more data coming (EOF), and it could just sit there waiting forever. Plus, each process can only handle so many file descriptors, so if you don't close them, you'll run out pretty quickly.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command must be built-in because it needs to change the shell process's own working directory. If implemented as an external command, it would change the directory in its child process only, leaving the shell's directory unchanged. This process isolation makes it impossible for external commands to modify the parent shell's state.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

I'd switch to using dynamic arrays that can grow using realloc instead of fixed-size arrays. This comes with some downsides though - the code gets more complex, there's some performance hit from memory management, and you have to handle more error cases. Probably best to start with a decent-sized array and just double it whenever you need more space.