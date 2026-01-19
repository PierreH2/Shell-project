#ifndef CLI_H
#define CLI_H

#include <stdio.h>

struct cli_ctx {
    FILE *input;
    int owns_file;     // 1 for fclose()
};

struct cli_ctx cli_parse(int argc, char **argv);
void cli_close(struct cli_ctx *ctx);

#endif
