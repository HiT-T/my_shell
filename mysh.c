#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFFERSIZE 1024       // Buffer size for input reading
#define ARGMAXNUM 128         // Max number of arguments per command

// Structure to manage line-based buffered input using read()
typedef struct {
    int fd;
    int pos;
    int len;
    char buf[BUFFERSIZE];
} lines_t;

// Global flags
bool is_interactive;          // True if running interactively
int last_status = -1;         // Stores status of last command

// Function declarations
char *readLine(lines_t *);
void welcome();
void goodbye();
void printPrompt();
bool isBuiltIn(char *);
void parseAndExecute(char *);
void runBuiltIn(char **, char *, char *);
void runExternal(char **, char *, char *);
void expandWildcard(const char *, char **, int *);
bool matchPattern(const char *, const char *);

// Initialize input buffer
void initLine(lines_t *l, int fd) {
    l->fd = fd;
    l->len = 0;
    l->pos = 0;
}

int main(int argc, char **argv) {
    // Reject more than one argument (unless input is not from terminal)
    if (argc > 2) {
        fprintf(stderr, "mysh: cannot hold more than one arguments\n");
        exit(EXIT_SUCCESS);
    }

    int fd = 0;
    is_interactive = (isatty(0) && argc == 1);    // Set interactive mode
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);             // Open script file
        if (fd < 0) { perror("mysh"); exit(EXIT_FAILURE); }
    }

    lines_t reader;
    initLine(&reader, fd);                        // Initialize input buffer

    welcome();
    char *line;
    while (true) {
        printPrompt();
        if (!(line = readLine(&reader))) break;   // Read command line
        parseAndExecute(line);                    // Execute command
        free(line);
    }
    goodbye();
    return 0;
}

// Read one line of input using low-level read()
char *readLine(lines_t *l) {
    char *line = NULL;
    int linelen = 0;
    while (1) {
        if (l->pos == l->len) {
            int bytes = read(l->fd, l->buf, BUFFERSIZE);
            if (bytes <= 0) return line;
            l->len = bytes;
            l->pos = 0;
        }
        int start = l->pos;
        while (l->pos < l->len && l->buf[l->pos] != '\n') l->pos++;
        int chunk = l->pos - start;
        line = realloc(line, linelen + chunk + 1);
        memcpy(line + linelen, l->buf + start, chunk);
        linelen += chunk;
        if (l->pos < l->len && l->buf[l->pos] == '\n') {
            l->pos++;
            line[linelen] = '\0';
            return line;
        }
    }
}

// Print welcome message (only in interactive mode)
void welcome() {
    if (is_interactive) printf("Welcome to my shell!\n");
}

// Print goodbye message (only in interactive mode)
void goodbye() {
    if (is_interactive) printf("mysh: exiting\n");
}

// Print prompt (only in interactive mode)
void printPrompt() {
    if (is_interactive) write(STDOUT_FILENO, "mysh> ", 6);
}

// Determine whether the command is a built-in command
bool isBuiltIn(char *cmd) {
    return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 || strcmp(cmd, "which") == 0 ||
           strcmp(cmd, "exit") == 0 || strcmp(cmd, "die") == 0;
}

// Parse input line, support conditionals, redirection, wildcards, pipelines
void parseAndExecute(char *line) {
    char *tokens[ARGMAXNUM];
    int tokc = 0;
    // Tokenize line, ignoring comments
    for (char *tok = strtok(line, " \t\n"); tok && tok[0] != '#'; tok = strtok(NULL, " \t\n"))
        tokens[tokc++] = tok;
    tokens[tokc] = NULL;
    if (tokc == 0) return;

    // Handle conditionals (and/or)
    bool run = true;
    int start = 0;
    if (strcmp(tokens[0], "and") == 0 || strcmp(tokens[0], "or") == 0) {
        if (last_status < 0) {
            fprintf(stderr, "mysh: conditionals should not occur in the first command\n");
            last_status = 1;
            return;
        }
        run = (strcmp(tokens[0], "and") == 0) ? last_status == 0 : last_status != 0;
        start = 1;
    }
    if (!run) return;

    // Parse command arguments
    char *args[ARGMAXNUM];
    int argc = 0;
    char *infile = NULL, *outfile = NULL;
    bool piping = false;
    int pipe_pos = -1;

    // Extract tokens: arguments, wildcards, redirection, pipe
    for (int i = start; i < tokc; i++) {
        if (strcmp(tokens[i], "<") == 0 && i + 1 < tokc) infile = tokens[++i];
        else if (strcmp(tokens[i], ">") == 0 && i + 1 < tokc) outfile = tokens[++i];
        else if (strcmp(tokens[i], "|") == 0) {
            piping = true;
            pipe_pos = argc;
            args[argc++] = NULL; // NULL separates left/right
        } else if (strchr(tokens[i], '*')) {
            expandWildcard(tokens[i], args, &argc); // Expand wildcard tokens
        } else {
            args[argc++] = tokens[i];
        }
    }
    args[argc] = NULL;
    if (argc == 0) { last_status = 1; return; }

    // Execute pipeline
    if (piping) {
        args[pipe_pos] = NULL;
        int pipefd[2];
        if (pipe(pipefd) < 0) { perror("pipe"); last_status = 1; return; }

        // First command in pipeline
        pid_t p1 = fork();
        if (p1 == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]); close(pipefd[1]);
            if (isBuiltIn(args[0])) runBuiltIn(args, infile, NULL);
            else runExternal(args, infile, NULL);
            exit(last_status);
        }

        // Second command in pipeline
        pid_t p2 = fork();
        if (p2 == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]); close(pipefd[1]);
            char **rhs = &args[pipe_pos + 1];
            if (isBuiltIn(rhs[0])) runBuiltIn(rhs, NULL, outfile);
            else runExternal(rhs, NULL, outfile);
            exit(last_status);
        }

        close(pipefd[0]); close(pipefd[1]);
        int status;
        wait(NULL);             // Wait first
        wait(&status);          // Then second
        last_status = (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : 1;

    } else {
        // No pipeline
        if (isBuiltIn(args[0])) runBuiltIn(args, infile, outfile);
        else runExternal(args, infile, outfile);
    }
}

// Execute built-in commands (supports redirection)
void runBuiltIn(char **args, char *infile, char *outfile) {
    if (infile) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) { perror("input"); exit(1); }
        dup2(fd, STDIN_FILENO); close(fd);
    }
    if (outfile) {
        int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0640);
        if (fd < 0) { perror("output"); exit(1); }
        dup2(fd, STDOUT_FILENO); close(fd);
    }

    // Handle built-in logic
    if (strcmp(args[0], "exit") == 0) { goodbye(); exit(EXIT_SUCCESS); }
    else if (strcmp(args[0], "die") == 0) {
        for (int i = 1; args[i]; i++) fprintf(stderr, "%s ", args[i]);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) { printf("%s\n", cwd); last_status = 0; }
        else { perror("pwd"); last_status = 1; }
    } else if (strcmp(args[0], "cd") == 0) {
        if (!args[1] || args[2]) { fprintf(stderr, "cd: wrong number of arguments\n"); last_status = 1; }
        else if (chdir(args[1]) != 0) { perror("cd"); last_status = 1; }
        else last_status = 0;
    } else if (strcmp(args[0], "which") == 0) {
        if (!args[1] || args[2]) { last_status = 1; return; }

        if (isBuiltIn(args[1])) { last_status = 1; return; } // Ignore built-ins

        const char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin"};
        for (int i = 0; i < 3; i++) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirs[i], args[1]);
            if (access(path, X_OK) == 0) {
                printf("%s\n", path);
                last_status = 0;
                return;
            }
        }
        fprintf(stderr, "which: command not found\n");
        last_status = 1;
    }
}

// Run external programs (execv)
void runExternal(char **args, char *infile, char *outfile) {
    pid_t pid = fork();
    if (pid == 0) {
        if (infile) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { perror("input"); exit(1); }
            dup2(fd, STDIN_FILENO); close(fd);
        }

        if (outfile) {
            int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            if (fd < 0) { perror("output"); exit(1); }
            dup2(fd, STDOUT_FILENO); close(fd);
        }

        if (strchr(args[0], '/')) execv(args[0], args); // Path given directly
        const char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin"};
        for (int i = 0; i < 3; i++) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirs[i], args[0]);
            execv(path, args);
        }
        perror("exec");
        exit(1);
    }
    int status;
    wait(&status);
    last_status = (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : 1;
}

// Expand wildcard tokens (foo*.sh)
void expandWildcard(const char *token, char **args, int *argc) {
    char dir[1024] = ".", pattern[256];
    const char *slash = strrchr(token, '/');
    if (slash) {
        strncpy(dir, token, slash - token);
        dir[slash - token] = '\0';
        strcpy(pattern, slash + 1);
    } else {
        strcpy(pattern, token);
    }
    DIR *dp = opendir(dir);
    if (!dp) { args[(*argc)++] = strdup(token); return; }
    struct dirent *entry;
    int matched = 0;
    while ((entry = readdir(dp))) {
        if (entry->d_name[0] == '.' && pattern[0] != '.') continue;
        if (matchPattern(entry->d_name, pattern)) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
            args[(*argc)++] = strdup(full);
            matched++;
        }
    }
    closedir(dp);
    if (!matched) args[(*argc)++] = strdup(token);
}

// Check if name matches wildcard pattern (prefix*suffix)
bool matchPattern(const char *name, const char *pattern) {
    const char *star = strchr(pattern, '*');
    if (!star) return strcmp(name, pattern) == 0;

    size_t prefix_len = star - pattern;
    const char *suffix = star + 1;
    size_t name_len = strlen(name);
    size_t suffix_len = strlen(suffix);

    if (name_len < prefix_len + suffix_len) return false;
    if (strncmp(name, pattern, prefix_len) != 0) return false;
    return strcmp(name + name_len - suffix_len, suffix) == 0;
}