#include <stdio.h>
#include <stdlib.h>

void syntax_error(const char *msg)
{
    fprintf(stderr, "[syntax_error stub] %s\n", msg);
    exit(1);
}
