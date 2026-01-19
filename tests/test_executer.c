#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdlib.h>

#include "parser/ast.h"
#include "executer/executer.h"

static char **make_argv(const char *a, const char *b)
{
    char **v = calloc(3, sizeof(char *));
    v[0] = strdup(a);
    v[1] = b ? strdup(b) : NULL;
    v[2] = NULL;
    return v;
}

Test(executer, simple_builtin_true)
{
    struct ast *cmd = ast_new_simple(make_argv("true", NULL));
    int st = exec_ast(cmd);
    cr_assert_eq(st, 0);
    ast_free(cmd);
}

Test(executer, simple_builtin_false)
{
    struct ast *cmd = ast_new_simple(make_argv("false", NULL));
    int st = exec_ast(cmd);
    cr_assert_eq(st, 1);
    ast_free(cmd);
}

Test(executer, list_returns_last_status)
{
    struct ast *cmd1 = ast_new_simple(make_argv("true", NULL));
    struct ast *cmd2 = ast_new_simple(make_argv("false", NULL));

    struct ast **items = calloc(2, sizeof(struct ast *));
    items[0] = cmd1;
    items[1] = cmd2;
    struct ast *list = ast_new_list(items, 2);

    int st = exec_ast(list);
    cr_assert_eq(st, 1);

    ast_free(list);
}

Test(executer, if_branch_taken)
{
    struct ast *cond = ast_new_simple(make_argv("true", NULL));
    struct ast *thenb = ast_new_simple(make_argv("false", NULL));
    struct ast *elseb = ast_new_simple(make_argv("true", NULL));

    struct ast *ifn = ast_new_if(cond, thenb, NULL, NULL, 0, elseb);

    int st = exec_ast(ifn);
    cr_assert_eq(st, 1); // false executed

    ast_free(ifn);
}
