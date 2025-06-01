#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#define MAX_COMMANDS_IN_PIPELINE 200
#define MAX_ARGS_PER_COMMAND 32
#define MAX_LINE 4096

char *trim_whitespace(char *str)
{
    char *end;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    *(end + 1) = 0;
    return str;
}

bool has_unmatched_quotes(const char *line)
{
    int single_q_count = 0;
    int double_q_count = 0;
    while (*line)
    {
        if (*line == '"')
            double_q_count++;
        else if (*line == '\'')
            single_q_count++;
        line++;
    }
    return (double_q_count % 2 != 0) || (single_q_count % 2 != 0);
}

char **parse_command_args(char *input)
{
    char **argv = malloc(sizeof(char *) * (MAX_ARGS_PER_COMMAND + 1));
    if (!argv)
    {
        perror("Shell: malloc");
        return NULL;
    }
    int argc = 0;
    char *p = input;
    while (*p && argc < MAX_ARGS_PER_COMMAND)
    {
        while (isspace((unsigned char)*p))
            p++;
        if (*p == '\0')
            break;

        char *start = p;
        char quote = 0;
        if (*p == '"' || *p == '\'')
        {
            quote = *p++;
            start = p;
            while (*p && *p != quote)
                p++;
        }
        else
        {
            while (*p && !isspace((unsigned char)*p))
                p++;
        }

        int len = p - start;
        char *arg = malloc(len + 1);
        if (!arg)
        {
            perror("Shell: malloc");
            for (int i = 0; i < argc; i++)
                free(argv[i]);
            free(argv);
            return NULL;
        }
        strncpy(arg, start, len);
        arg[len] = '\0';
        argv[argc++] = arg;

        if (quote && *p == quote)
            p++;
        else if (!quote && isspace((unsigned char)*p))
            p++;
        else if (*p != '\0' && !isspace((unsigned char)*p) && !quote)
        {
            /* In case of unquoted argument followed immediately by non-space, non-null char (should not happen if line ends or next is delimiter) */
            /* Or if *p became null after quote processing, this path is not taken */
        }
    }
    argv[argc] = NULL;
    return argv;
}

void free_args(char **argv)
{
    if (!argv)
        return;
    for (int i = 0; argv[i]; i++)
        free(argv[i]);
    free(argv);
}

int main()
{
    char line[MAX_LINE];

    while (1)
    {
        if (isatty(STDIN_FILENO))
        {
            printf("Shell> ");
        }
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = 0;
        if (has_unmatched_quotes(line))
        {
            fprintf(stderr, "Shell: unmatched quote\n");
            continue;
        }

        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '\0')
            continue;

        if (trimmed[0] == '|' || (strlen(trimmed) > 0 && trimmed[strlen(trimmed) - 1] == '|'))
        {
            fprintf(stderr, "Shell: Syntax error near unexpected token `|'\n");
            continue;
        }

        char *commands_storage[MAX_COMMANDS_IN_PIPELINE];
        int num_commands = 0;
        char *line_copy = strdup(trimmed);
        if (!line_copy)
        {
            perror("Shell: strdup");
            continue;
        }

        char *token = strtok(line_copy, "|");
        while (token && num_commands < MAX_COMMANDS_IN_PIPELINE)
        {
            char *segment = trim_whitespace(token);
            if (segment[0] == '\0')
            {
                fprintf(stderr, "Shell: Syntax error near unexpected token `|'\n");
                num_commands = -1;
                break;
            }
            commands_storage[num_commands++] = segment;
            token = strtok(NULL, "|");
        }

        if (num_commands <= 0)
        {
            free(line_copy);
            continue;
        }

        if (strcmp(commands_storage[0], "exit") == 0)
        {
            if (num_commands > 1)
            {
                fprintf(stderr, "Shell: \"exit\" cannot be part of a pipeline\n");
                free(line_copy);
                continue;
            }
            free(line_copy);
            break;
        }

        int pipe_descriptors[MAX_COMMANDS_IN_PIPELINE - 1][2];
        for (int i = 0; i < num_commands - 1; i++)
        {
            if (pipe(pipe_descriptors[i]) < 0)
            {
                perror("Shell: pipe");
                free(line_copy);
                goto cleanup_pipes_and_continue;
            }
        }

        pid_t pids[MAX_COMMANDS_IN_PIPELINE];
        bool pipeline_error = false;

        for (int i = 0; i < num_commands; i++)
        {
            char **argv = parse_command_args(commands_storage[i]);
            if (!argv || !argv[0])
            {
                fprintf(stderr, "Shell: parsing error or empty command for: %s\n", commands_storage[i]);
                free_args(argv);
                pipeline_error = true;
                break;
            }

            pids[i] = fork();
            if (pids[i] < 0)
            {
                perror("Shell: fork");
                free_args(argv);
                pipeline_error = true;
                break;
            }

            if (pids[i] == 0)
            {
                if (i > 0)
                {
                    if (dup2(pipe_descriptors[i - 1][0], STDIN_FILENO) < 0)
                    {
                        perror("Shell: dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                if (i < num_commands - 1)
                {
                    if (dup2(pipe_descriptors[i][1], STDOUT_FILENO) < 0)
                    {
                        perror("Shell: dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }
                for (int j = 0; j < num_commands - 1; j++)
                {
                    close(pipe_descriptors[j][0]);
                    close(pipe_descriptors[j][1]);
                }
                execvp(argv[0], argv);
                perror("Shell: execvp");
                free_args(argv);
                free(line_copy);
                exit(EXIT_FAILURE);
            }
            free_args(argv);
        }

        for (int i = 0; i < num_commands - 1; i++)
        {
            close(pipe_descriptors[i][0]);
            close(pipe_descriptors[i][1]);
        }

        if (!pipeline_error)
        {
            for (int i = 0; i < num_commands; i++)
            {
                waitpid(pids[i], NULL, 0);
            }
        }
        else
        {
            for (int i = 0; i < num_commands; i++)
            {
                if (pids[i] > 0)
                {
                    kill(pids[i], SIGTERM);
                    waitpid(pids[i], NULL, 0);
                }
            }
        }

    cleanup_pipes_and_continue:
        free(line_copy);
    }
    return 0;
}