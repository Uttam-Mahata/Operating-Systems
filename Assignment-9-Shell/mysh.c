#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/wait.h>   
#include <sys/types.h>  
#include <errno.h>

#define MAX_LINE_LEN 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 16
#define TOK_DELIM " \t\r\n\a"

int builtin_cd(char **args);
int builtin_pwd(char **args);
int builtin_clear(char **args);
int builtin_exit(char **args);

char *builtin_str[] = {
    "cd",
    "pwd",
    "clear",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &builtin_cd,
    &builtin_pwd,
    &builtin_clear,
    &builtin_exit
};

int num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int builtin_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mysh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("mysh: cd failed");
        }
    }
    return 1;
}

int builtin_pwd(char **args) {
    char cwd[MAX_LINE_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("mysh: pwd failed");
        return 0;
    }
    return 1;
}

int builtin_clear(char **args) {
    printf("\033[H\033[J");
    return 1;
}

int builtin_exit(char **args) {
    printf("Exiting mysh.\n");
    return 0;
}

int launch_process(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("mysh: command execution failed");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_FAILURE);

    } else if (pid == -1) {
        perror("mysh: fork failed");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int execute_command(char **args) {
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return launch_process(args);
}

int parse_command(char *line, char **args) {
    int position = 0;
    char *token;

    token = strtok(line, TOK_DELIM);
    while (token != NULL) {
        args[position] = token;
        position++;

        if (position >= MAX_ARGS) {
            fprintf(stderr, "mysh: Too many arguments.\n");
            args[MAX_ARGS - 1] = NULL;
            return position;
        }

        token = strtok(NULL, TOK_DELIM);
    }
    args[position] = NULL;
    return position;
}

int split_line_semicolon(char *line, char **commands) {
    int position = 0;
    char *token;

    token = strtok(line, ";");
    while (token != NULL && position < MAX_COMMANDS -1) {
        while (*token == ' ' || *token == '\t') token++;

        if (*token != '\0') {
             commands[position] = token;
             position++;
        }

        token = strtok(NULL, ";");
    }
    commands[position] = NULL;
    return position;
}

char *read_line(FILE *input_stream) {
    char *line = NULL;
    size_t bufsize = 0;

    if (getline(&line, &bufsize, input_stream) == -1) {
        if (feof(input_stream)) {
            free(line);
            return NULL;
        } else {
            perror("mysh: getline failed");
            free(line);
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

void run_shell(FILE *input_stream, int interactive_mode) {
    char *line = NULL;
    char *line_copy = NULL;
    char *commands[MAX_COMMANDS];
    char *args[MAX_ARGS];
    int status = 1;
    int i;

    do {
        if (interactive_mode) {
            printf("mysh> ");
            fflush(stdout);
        }

        line = read_line(input_stream);

        if (line == NULL) {
            if (interactive_mode) printf("\n");
            status = 0;
            continue;
        }

        line_copy = strdup(line);
        if (!line_copy) {
             perror("mysh: strdup failed");
             free(line);
             continue;
        }

        int num_commands = split_line_semicolon(line_copy, commands);

        for (i = 0; i < num_commands && status; i++) {
            char *cmd_copy = strdup(commands[i]);
             if (!cmd_copy) {
                 perror("mysh: strdup failed");
                 status = 0;
                 break;
             }

            parse_command(cmd_copy, args);

            status = execute_command(args);

            free(cmd_copy);

            if (!status) {
                break;
            }
        }

        free(line);
        free(line_copy);

    } while (status);
}

int main(int argc, char **argv) {
    FILE *input_stream = stdin;
    int interactive_mode = 1;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [script_file]\n", argv[0]);
        return EXIT_FAILURE;
    } else if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("mysh: Error opening script file");
            return EXIT_FAILURE;
        }
        interactive_mode = 0;
        printf("Executing commands from %s\n", argv[1]);
    } else {
        printf("Welcome to mysh (Primitive Shell)\n");
        printf("Enter commands, use 'exit' to quit.\n");
    }

    run_shell(input_stream, interactive_mode);

    if (input_stream != stdin) {
        fclose(input_stream);
    }

    return EXIT_SUCCESS;
}