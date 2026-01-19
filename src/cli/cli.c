#include "cli.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *out)
{
    fprintf(out, "Usage: 42sh [OPTIONS] [SCRIPT] [ARGUMENTS...]\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -c \"SCRIPT\"   read commands from string\n");
}

static void die_cli(const char *msg)
{
    if (msg)
        fprintf(stderr, "42sh: %s\n", msg);
    usage(stderr);
    exit(SHELL_ERR_CLI);
}

static FILE *open_mem_script(const char *s)
{
#if defined(__GLIBC__) || defined(__APPLE__)
    FILE *f = fmemopen((void *)s, strlen(s), "r");
    if (!f)
        die_cli("cannot open -c script");
    return f;
#else
    // Fallback portable: write to tmpfile
    FILE *f = tmpfile();
    if (!f)
        die_cli("cannot create tmpfile for -c script");
    fputs(s, f);
    rewind(f);
    return f;
#endif
}

struct cli_ctx cli_parse(int argc, char **argv)
{
    struct cli_ctx ctx;
    ctx.input = NULL;
    ctx.owns_file = 0;

    if (argc >= 2 && strcmp(argv[1], "-c") == 0) {
        if (argc < 3)
            die_cli("missing argument after -c");
        ctx.input = open_mem_script(argv[2]);
        ctx.owns_file = 1;
        return ctx;
    }

    if (argc >= 2) {
        // treat argv[1] as script file (ignore ARGUMENTS for step1)
        ctx.input = fopen(argv[1], "r");
        if (!ctx.input) {
            fprintf(stderr, "42sh: cannot open file: %s\n", argv[1]);
            exit(SHELL_ERR_CLI);
        }
        ctx.owns_file = 1;
        return ctx;
    }

    ctx.input = stdin;
    ctx.owns_file = 0;
    return ctx;
}

void cli_close(struct cli_ctx *ctx)
{
    if (ctx->owns_file && ctx->input) {
        fclose(ctx->input);
        ctx->input = NULL;
        ctx->owns_file = 0;
    }
}
