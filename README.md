# myshell

`myshell` is a simple text-based Unix shell written in C.

The project implements a simple command-line shell using Linux/POSIX process API.  
External programs are executed using `fork()` and `execvp()`.  
The project does not use `system()`, `popen()` or any other function that delegates command execution to the system shell.

## Authors

- KC
- GC
- KP

## Project files

The archive contains:

```text
myshell.c    - source code of the shell
Makefile     - build script
README.md    - project documentation
test.sh      - basic test script
```

After compilation, the executable file `myshell` is created.

## Compilation

To compile the project, run:

```bash
make
```

This creates the executable:

```bash
./myshell
```

To clean generated files, run:

```bash
make clean
```

Manual compilation is also possible:

```bash
gcc -Wall -Wextra -std=c11 myshell.c -o myshell
```

## Running

Run the shell with:

```bash
./myshell
```

The shell can also execute commands from a script file passed as an argument:

```bash
./myshell script_file
```

The interactive prompt is:

```text
myshell>
```

To exit the shell, send EOF:

```text
Ctrl+D
```

## Running with syslog logging

To enable command logging to syslog, run:

```bash
./myshell -l
```

or:

```bash
./myshell --syslog
```

In this mode, each entered command is written to syslog.

## Testing

A basic test script is included. It can be run directly:

```bash
./test.sh
```

or through Makefile:

```bash
make test
```

Before running it, make sure it is executable:

```bash
chmod +x test.sh
```

The script tests:

- compilation,
- simple external commands,
- output redirection with `>`,
- output redirection with `>>`,
- the `cd` command,
- background processes,
- the `jobs` command,
- command history file,
- script interpreter mode using a shebang line.

The shell can also be tested manually, for example:

```bash
./myshell
```

Example commands:

```bash
pwd
ls -l
cd /tmp
pwd
echo hello > out.txt
cat out.txt
echo world >> out.txt
cat out.txt
sleep 10 &
jobs
```

## Implemented functionality

The following functionality is implemented.

### 1. Reading commands from standard input

The shell reads commands line by line from standard input using `getline()`.

### 2. Simple command parsing

Each input line is split into words separated by spaces, tabs or newline characters.

The first word is treated as the program name.  
The remaining words are treated as program arguments.

Example:

```bash
ls -l /tmp
```

is parsed as:

```text
program: ls
arguments: -l /tmp
```

### 3. Executing external programs

External programs are executed using:

```c
fork()
execvp()
waitpid()
```

The shell does not use:

```c
system()
popen()
```

or the system shell for command execution.

### 4. PATH support

The shell uses `execvp()`, so programs are searched using the `PATH` environment variable.

Example:

```bash
ls
```

works without writing:

```bash
/bin/ls
```

### 5. Foreground execution

By default, the shell waits for the executed program to finish.

Example:

```bash
sleep 5
```

The shell waits until the command finishes.

### 6. Background execution

If the last word of a command is `&`, the command is executed in the background.

Example:

```bash
sleep 30 &
```

The shell immediately returns to the prompt and does not wait for the process to finish.

### 7. Background jobs list

The built-in command:

```bash
jobs
```

prints active background processes.

For each process, the shell displays:

- PID,
- start time,
- command text.

Example output:

```text
--- background jobs ---
PID: 12345 | started: 2026-05-09 15:20:10 | command: sleep 30 &
-----------------------
```

Finished background processes are removed from the list automatically using `waitpid()` with `WNOHANG`.

### 8. Changing the current directory

The shell implements the built-in command:

```bash
cd
```

Examples:

```bash
cd /tmp
cd
```

`cd /tmp` changes the shell working directory to `/tmp`.

`cd` without arguments changes the directory to the user's home directory.

The implementation uses:

```c
chdir()
```

### 9. Syslog logging

When started with:

```bash
./myshell -l
```

or:

```bash
./myshell --syslog
```

the shell logs every entered command to syslog using:

```c
syslog()
```

### 10. Command history

The shell stores the last 20 non-empty commands.

The history is saved in the user's home directory:

```text
~/.myshell_history
```

The history is loaded when the shell starts, so it survives closing and restarting the shell.

### 11. Printing history with SIGQUIT

When the shell receives the `SIGQUIT` signal, it prints the command history to standard output.

In most terminals, `SIGQUIT` can be sent using:

```text
Ctrl+\
```

Example output:

```text
--- history ---
 1: echo hello
 2: pwd
 3: ls -l
---------------
```

### 12. EOF handling

The shell exits when it receives EOF.

In an interactive terminal this can be done using:

```text
Ctrl+D
```

This also allows the shell to process commands from a file.

Examples:

```bash
./myshell < commands.txt
./myshell commands.txt
```

### 13. Script usage

The shell can be used as an interpreter for simple scripts.

Example script:

```bash
#!/full/path/to/myshell
echo script started
pwd
ls
echo script finished
```

The script must be executable:

```bash
chmod +x script
```

Then it can be run with:

```bash
./script
```

### 14. Output redirection with `>`

The shell supports redirecting standard output to a file and overwriting the file.

Example:

```bash
echo hello > file.txt
```

The implementation uses:

```c
open()
dup2()
close()
```

with `O_TRUNC`.

### 15. Output redirection with `>>`

The shell supports redirecting standard output to a file and appending to the file.

Example:

```bash
echo hello >> file.txt
```

The implementation uses:

```c
open()
dup2()
close()
```

with `O_APPEND`.

## Built-in commands

The shell implements the following built-in commands:

```text
cd
jobs
```

These commands are handled directly by the shell process.

## Special operators

The shell recognizes the following special operators:

```text
&
>
>>
```

Meaning:

```text
&   run command in the background
>   redirect stdout to a file and overwrite it
>>  redirect stdout to a file and append to it
```

## Program options

The executable supports the following startup options:

```text
-l
--syslog
```

Both options enable command logging to syslog. The program may also receive one script file path as an argument.

## Limitations

This is a simple educational shell, not a full Bash replacement.

The following features are not implemented:

- pipes, for example `ls | grep txt`,
- input redirection, for example `sort < file.txt`,
- stderr redirection, for example `2> errors.txt`,
- quote handling as argument grouping, for example `echo "hello world"`,
- wildcard expansion, for example `ls *.c`,
- shell variables, for example `echo $HOME`,
- aliases,
- command separators with `;`,
- logical operators `&&` and `||`,
- job control commands such as `fg` and `bg`,
- command history navigation with arrow keys,
- an internal `exit` command.

The shell exits using EOF, for example with `Ctrl+D`.

## Example session

```text
myshell> pwd
/home/student
myshell> ls -l
myshell> cd /tmp
myshell> pwd
/tmp
myshell> echo hello > out.txt
myshell> cat out.txt
hello
myshell> echo world >> out.txt
myshell> cat out.txt
hello
world
myshell> sleep 20 &
[background] pid: 12345
myshell> jobs
--- background jobs ---
PID: 12345 | started: 2026-05-09 15:20:10 | command: sleep 20 &
-----------------------
myshell>
```
