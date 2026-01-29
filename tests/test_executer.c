#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

static char **make_argv_n(const char *a, const char *b, const char *c)
{
    char **v = calloc(4, sizeof(char *));
    v[0] = strdup(a);
    v[1] = b ? strdup(b) : NULL;
    v[2] = c ? strdup(c) : NULL;
    v[3] = NULL;
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

Test(executer, echo_with_output_redirection)
{
    char tmpfile[] = "/tmp/test_redir_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    struct redirection r;
    r.type = REDIR_OUT;
    r.target = strdup(tmpfile);
    r.fd = 1;

    char **argv = make_argv("echo", "hello");
    struct ast *cmd = ast_new_simple_with_redirs(argv, calloc(1, sizeof(struct redirection)), 1);
    cmd->as.simple.redirs[0] = r;

    int st = exec_ast(cmd);
    cr_assert_eq(st, 0);

    // Check file content
    FILE *f = fopen(tmpfile, "r");
    char buf[100];
    fgets(buf, sizeof(buf), f);
    cr_assert(strstr(buf, "hello") != NULL);
    fclose(f);

    unlink(tmpfile);
    ast_free(cmd);
}

Test(executer, echo_with_append_redirection)
{
    char tmpfile[] = "/tmp/test_append_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    // Write initial content
    FILE *f = fopen(tmpfile, "w");
    fprintf(f, "line1\n");
    fclose(f);

    struct redirection r;
    r.type = REDIR_APPEND;
    r.target = strdup(tmpfile);
    r.fd = 1;

    char **argv = make_argv("echo", "line2");
    struct ast *cmd = ast_new_simple_with_redirs(argv, calloc(1, sizeof(struct redirection)), 1);
    cmd->as.simple.redirs[0] = r;

    int st = exec_ast(cmd);
    cr_assert_eq(st, 0);

    // Check file content has both lines
    f = fopen(tmpfile, "r");
    char buf[200];
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "line1") != NULL);
    cr_assert(strstr(buf, "line2") != NULL);

    unlink(tmpfile);
    ast_free(cmd);
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
