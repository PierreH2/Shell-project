#include "builtins.h"
#include <stdio.h>
#include <string.h>

static void echo_print_escaped(const char *s)
{
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\\' && s[i + 1]) {
            char n = s[i + 1];
            if (n == 'n') {
                putchar('\n');
                i++;
                continue;
            }
            if (n == 't') {
                putchar('\t');
                i++;
                continue;
            }
            if (n == '\\') {
                putchar('\\');
                i++;
                continue;
            }
        }
        putchar(s[i]);
    }
}

static int builtin_true(char **argv)
{
    (void)argv;
    return 0;
}

static int builtin_false(char **argv)
{
    (void)argv;
    return 1;
}

static int builtin_echo(char **argv)
{
    int newline = 1;
    int interpret = 0; // -e enables, -E disables

    int i = 1;
    while (argv[i]) {
        if (strcmp(argv[i], "-n") == 0) {
            newline = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-e") == 0) {
            interpret = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-E") == 0) {
            interpret = 0;
            i++;
            continue;
        }
        break;
    }

    int first = 1;
    for (; argv[i]; i++) {
        if (!first)
            putchar(' ');
        first = 0;

        if (interpret)
            echo_print_escaped(argv[i]);
        else
            fputs(argv[i], stdout);
    }

    if (newline)
        putchar('\n');

    fflush(stdout);
    return 0;
}

int try_builtin(char **argv, int *out_status)
{
    if (!argv || !argv[0])
        return 0;

    if (strcmp(argv[0], "true") == 0) {
        *out_status = builtin_true(argv);
        return 1;
    }
    if (strcmp(argv[0], "false") == 0) {
        *out_status = builtin_false(argv);
        return 1;
    }
    if (strcmp(argv[0], "echo") == 0) {
        *out_status = builtin_echo(argv);
        return 1;
    }
    return 0;
}
