# Shell
This is the implementation of a simple shell.
* System commands (may or may not include a pathname) such as 'ls -l' are supported, and it also supports 
background and foreground processes.
* Built-in commands such as 'cd, dir, clr, wait, hist, exit' are supported.
  * 'cd <directory>' - changes the current directory to '<directory>'
  * 'dir' - prints the current working directory
  * 'clr' - clears the screen
  * 'wait' - waits until all background processes are complete
  * 'hist' - prints up to the 10 most recently entered commands in your shell.
    <br />
    * 'hist -set num' - set the size of the history buffer to num.
    * '! number' - repeats the command that is entered on turn 'number' (1 for the 1st command in history).
    * '! string' - repeats the command whose first two letter is same as 'string' (cl for clr).
    <br />
  * 'exit' - terminates shell.
* Pipe operator is supported. However, it doesn't support multiple pipes.

## How to Use It?
* Clone the repo.
* Compile and run.
