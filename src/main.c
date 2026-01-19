#include "cli/cli.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "executer/executer.h"
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct cli_ctx ctx = cli_parse(argc, argv);

    struct lexer lx;
    lexer_init(&lx, ctx.input);

    struct ast *root = parse_input(&lx);

    int status = 0;
    if (root) {
        status = exec_ast(root);
        ast_free(root);
    }

    cli_close(&ctx);
    return status;
}
