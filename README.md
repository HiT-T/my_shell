# CS214 Project III: My Shell

## Authors
| **Name** | **NetID** |
|----------|-------------|
| [Vishal Nagamalla](https://github.com/Vishal-Nagamalla) | vn218 |
| [Yuhan Li](https://github.com/HiT-T) | yl2355 |

---

## Description

This program, `mysh.c`, is a simple command-line shell that supports both interactive and batch modes. It reads one command per line and executes it. Key features include:

- **Built-in commands**: `cd`, `pwd`, `which`, `exit`, `die`
- **Redirection**: Supports `<` for input redirection and `>` for output redirection  
- **Pipelines**: Allows a single pipeline (i.e. `cmd1 | cmd2`)  
- **Wildcards**: Expands tokens containing `*` by matching file names in the specified directory  
- **Conditionals**: Executes commands conditionally using `and` (if previous succeeded) or `or` (if previous failed)

In **interactive mode**, the shell prints a welcome message, a prompt (`mysh> `), and a goodbye message upon exit. In **batch mode**, it does none of these, and simply reads commands from a file or a non-terminal input stream (e.g., `cat script.sh | ./mysh`).

---

## Design Choices

- **Input Reading**: Uses `read()` directly into an internal buffer, accumulating data line by line without relying on higher-level standard I/O functions.
- **Tokenization**: Splits commands by whitespace and special symbols (`<`, `>`, `|`), ignoring anything after `#` as a comment.
- **Built-In Detection**: Recognizes the commands `cd`, `pwd`, `which`, `exit`, and `die` as built-ins; all other commands are executed via `fork()` and `execv()`.
- **Redirection**: Uses `dup2()` in the child process to set stdin and stdout to user-specified files, creating/truncating output files with mode `0640`.
- **Pipes**: Supports a single pipeline of two processes (`cmd1 | cmd2`) by creating a pipe with `pipe()`, then using `dup2()` to connect their file descriptors.
- **Wildcards**: Looks for `*` in a token. If found, it scans the appropriate directory (or current directory if none is specified in the path), matching files using a prefix/suffix approach. If no matches are found, the original token is passed on.
- **Conditionals**: If a command starts with `and` or `or`, it checks the exit status of the previous command to decide whether to run the current one.

---

## Testing Strategy

- **Functional Tests**:  
  - **test1.sh**: Checks basic built-ins (`cd`, `pwd`, `which`, `exit`), external commands (`ls`, `echo`), and ensures correct batch-mode flow.  
  - **test2.sh**: Verifies redirection (`<`, `>`) and a single pipeline (`|`). Includes reading from an `input.txt` file and writing to `out.txt`.  
  - **test3.sh**: Exercises wildcards (`*.c`, `*.txt`) and conditionals (`and`, `or`) to confirm success/fail logic.

- **Interactive Checks**:  
  - Ran `./mysh` directly, confirmed it prints a welcome message, followed by `mysh> ` prompt, and a goodbye message upon `exit`.

- **Edge Cases**:  
  - Empty lines or lines with only `# comment`.  
  - Attempted redirection with invalid files.  
  - `cd` with missing or extra arguments.  
  - Wildcard tokens that match no files.

- **Memory Checking**:  
  - Verified that the shell does not leak memory excessively by running it on multiple commands and using short-lived processes; also used `valgrind` in an optional step to confirm no major leaks.

---

## Compilation & Execution

To compile the project:

```bash
make