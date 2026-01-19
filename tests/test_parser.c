#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <string.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"

static struct ast *parse_from_str(const char *s)
{
    FILE *f = fmemopen((void *)s, strlen(s), "r");
    cr_assert_not_null(f);

    struct lexer lx;
    lexer_init(&lx, f);

    struct ast *root = parse_input(&lx);
    fclose(f);
    return root;
}

Test(parser, simple_command)
{
    struct ast *ast = parse_from_str("echo hello");

    cr_assert_not_null(ast);
    cr_assert_eq(ast->type, AST_LIST);
    cr_assert_eq(ast->as.list.len, 1);

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->type, AST_SIMPLE);
    cr_assert_str_eq(cmd->as.simple.argv[0], "echo");
    cr_assert_str_eq(cmd->as.simple.argv[1], "hello");
    cr_assert_null(cmd->as.simple.argv[2]);

    ast_free(ast);
}

Test(parser, list_of_commands)
{
    struct ast *ast = parse_from_str("echo a; echo b; echo c");

    cr_assert_eq(ast->type, AST_LIST);
    cr_assert_eq(ast->as.list.len, 3);

    cr_assert_str_eq(ast->as.list.items[0]->as.simple.argv[0], "echo");
    cr_assert_str_eq(ast->as.list.items[1]->as.simple.argv[1], "b");
    cr_assert_str_eq(ast->as.list.items[2]->as.simple.argv[1], "c");

    ast_free(ast);
}

Test(parser, if_then_else)
{
    struct ast *ast = parse_from_str(
        "if true; then echo yes; else echo no; fi");

    cr_assert_eq(ast->type, AST_LIST);
    cr_assert_eq(ast->as.list.len, 1);

    struct ast *ifn = ast->as.list.items[0];
    cr_assert_eq(ifn->type, AST_IF);

    cr_assert_not_null(ifn->as.ifnode.cond);
    cr_assert_not_null(ifn->as.ifnode.then_branch);
    cr_assert_not_null(ifn->as.ifnode.else_branch);

    ast_free(ast);
}

Test(parser, syntax_error_missing_fi, .exit_code = 2)
{
    parse_from_str("if true; then echo fail");
}
