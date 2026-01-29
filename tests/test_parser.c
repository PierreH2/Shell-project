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

Test(parser, simple_output_redirection)
{
    struct ast *ast = parse_from_str("echo hello > file.txt");

    cr_assert_eq(ast->type, AST_LIST);
    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->type, AST_SIMPLE);

    cr_assert_eq(cmd->as.simple.redir_len, 1);
    cr_assert_not_null(cmd->as.simple.redirs);
    
    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_OUT);
    cr_assert_str_eq(r->target, "file.txt");
    cr_assert_eq(r->fd, 1); // stdout

    ast_free(ast);
}

Test(parser, simple_input_redirection)
{
    struct ast *ast = parse_from_str("cat < input.txt");

    cr_assert_eq(ast->type, AST_LIST);
    struct ast *cmd = ast->as.list.items[0];

    cr_assert_eq(cmd->as.simple.redir_len, 1);
    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_IN);
    cr_assert_str_eq(r->target, "input.txt");
    cr_assert_eq(r->fd, 0); // stdin

    ast_free(ast);
}

Test(parser, append_redirection)
{
    struct ast *ast = parse_from_str("echo test >> output.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_APPEND);
    cr_assert_str_eq(r->target, "output.txt");

    ast_free(ast);
}

Test(parser, clobber_redirection)
{
    struct ast *ast = parse_from_str("echo test >| output.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_CLOBBER);

    ast_free(ast);
}

Test(parser, rdwr_redirection)
{
    struct ast *ast = parse_from_str("command <> file.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_RDWR);
    cr_assert_str_eq(r->target, "file.txt");

    ast_free(ast);
}

Test(parser, out_err_redirection)
{
    struct ast *ast = parse_from_str("cmd >& file.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_OUT_ERR);

    ast_free(ast);
}

Test(parser, in_err_redirection)
{
    struct ast *ast = parse_from_str("cmd <& 3");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_IN_ERR);
    cr_assert_str_eq(r->target, "3");

    ast_free(ast);
}

Test(parser, ionumber_redirection)
{
    struct ast *ast = parse_from_str("echo 2> errors.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 1);

    struct redirection *r = &cmd->as.simple.redirs[0];
    cr_assert_eq(r->type, REDIR_OUT);
    cr_assert_eq(r->fd, 2); // stderr
    cr_assert_str_eq(r->target, "errors.txt");

    ast_free(ast);
}

Test(parser, multiple_redirections)
{
    struct ast *ast = parse_from_str("cmd < in.txt > out.txt 2> err.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_eq(cmd->as.simple.redir_len, 3);

    cr_assert_eq(cmd->as.simple.redirs[0].type, REDIR_IN);
    cr_assert_eq(cmd->as.simple.redirs[1].type, REDIR_OUT);
    cr_assert_eq(cmd->as.simple.redirs[2].type, REDIR_OUT);
    cr_assert_eq(cmd->as.simple.redirs[2].fd, 2);

    ast_free(ast);
}

Test(parser, redirection_with_multiple_args)
{
    struct ast *ast = parse_from_str("echo arg1 arg2 > file.txt");

    struct ast *cmd = ast->as.list.items[0];
    cr_assert_str_eq(cmd->as.simple.argv[0], "echo");
    cr_assert_str_eq(cmd->as.simple.argv[1], "arg1");
    cr_assert_str_eq(cmd->as.simple.argv[2], "arg2");
    cr_assert_null(cmd->as.simple.argv[3]);
    cr_assert_eq(cmd->as.simple.redir_len, 1);

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
