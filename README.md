# Custom Linux Shell in C

A lightweight, fully functional Unix-like shell written in C. This project demonstrates core Operating System concepts including process management, inter-process communication (IPC), file descriptor manipulation, and asynchronous signal handling.

## Key Features

- **Command Execution:** Executes standard Linux commands using `fork()` and `execvp()`.
- **Pipeline Support (`|`):** Supports chaining multiple commands together via pipes (e.g., `ls -l | grep .c | wc -l`).
- **I/O Redirection:**
  - Output redirection: `>` (overwrite) and `>>` (append).
  - Input redirection: `<`.
  - Standard Error redirection: `2>`.
- **Background Processing (`&`):** Supports running commands in the background. The shell asynchronously reaps zombie processes using the `SIGCHLD` signal handler.
- **Signal Handling:** Custom handler for `SIGINT` (Ctrl+C) to prevent the shell from terminating, mimicking bash behavior.
- **Command History:** Supports recalling the last executed command using the Up-Arrow key or the `!!` command.
- **Built-in Commands:**
  - `cd` - Change directory.
  - `status` - View the exit status of the last executed foreground command.
  - `prompt = <new_prompt>` - Customize the shell prompt string.
  - `quit` - Exit the shell.
- **String Parsing:** Correctly parses commands with spaces wrapped in quotes (`"..."`).

## Getting Started

### Prerequisites

- GCC (GNU Compiler Collection)
- A Linux/Unix environment

### Compilation

Clone the repository and compile the source code using `gcc`:

```bash
git clone [https://github.com/arielelazam/custom-linux-shell.git](https://github.com/arielelazam/custom-linux-shell.git)
cd custom-linux-shell
gcc shell_2.c -o myshell

```

### Usage

Run the compiled executable to start the shell:

```bash
./myshell

```

Once inside, you can run standard commands, pipe them, or test the built-in features:

```bash
hello: ls -l | grep shell
hello: echo "Test Error" 2> error.log
hello: sleep 5 &
hello: status
hello: quit
```
