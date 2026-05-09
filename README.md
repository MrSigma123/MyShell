# myshell

`myshell` is a simple text-based Unix shell written in C.

The shell reads commands from standard input, splits them into words separated by spaces,
and executes external programs using `fork()` and `execvp()`. It also supports background
processes, command history, output redirection, changing the working directory, and optional
syslog logging.

## Files

The project contains:

```text
myshell.c
Makefile
README.md
