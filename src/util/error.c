#include "error.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>

void syntax_error(int line, int col, const char *msg)
{
    if (!msg)
        msg = "syntax error";
    fprintf(stderr, "42sh: %s at %d:%d\n", msg, line, col);
    exit(SHELL_ERR_SYNTAX);
}
