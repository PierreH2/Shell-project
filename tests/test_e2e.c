#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <string.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "executer/executer.h"

static int run_script(const char *s)
{
    FILE *f = fmemopen((void *)s, strlen(s), "r");
    cr_assert_not_null(f);

    struct lexer lx;
    lexer_init(&lx, f);

    struct ast *root = parse_input(&lx);
    int st = exec_ast(root);

    ast_free(root);
    fclose(f);
    return st;
}

static void redirect_all(void)
{
    cr_redirect_stdout();
    cr_redirect_stderr();
}

Test(e2e, echo_hello, .init = redirect_all)
{
    int st = run_script("echo hello");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello\n");
}

Test(e2e, multiple_commands, .init = redirect_all)
{
    int st = run_script("echo a; echo b");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("a\nb\n");
}

Test(e2e, if_else, .init = redirect_all)
{
    int st = run_script("if false; then echo no; else echo yes; fi");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("yes\n");
}

Test(e2e, elif_chain, .init = redirect_all)
{
    int st = run_script(
        "if false; then echo no;"
        "elif true; then echo ok;"
        "else echo bad; fi");

    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("ok\n");
}

Test(e2e, quotes_and_comments, .init = redirect_all)
{
    int st = run_script(
        "echo 'hello world' # comment\n"
        "echo ok\n");   

    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello world\nok\n");
}
