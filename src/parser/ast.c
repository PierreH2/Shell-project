#include "ast.h"
#include <stdlib.h>

static void free_argv(char **argv)
{
    if (!argv) return;
    for (size_t i = 0; argv[i]; i++)
        free(argv[i]);
    free(argv);
}

static void free_redirs(struct redirection *redirs, size_t len)
{
    if (!redirs) return;
    for (size_t i = 0; i < len; i++)
        free(redirs[i].target);
    free(redirs);
}

struct ast *ast_new_simple(char **argv)
{
    struct ast *n = calloc(1, sizeof(*n));
    if (!n) abort();
    n->type = AST_SIMPLE;
    n->as.simple.argv = argv;
    n->as.simple.redirs = NULL;
    n->as.simple.redir_len = 0;
    return n;
}

struct ast *ast_new_simple_with_redirs(char **argv, struct redirection *redirs, size_t redir_len)
{
    struct ast *n = calloc(1, sizeof(*n));
    if (!n) abort();
    n->type = AST_SIMPLE;
    n->as.simple.argv = argv;
    n->as.simple.redirs = redirs;
    n->as.simple.redir_len = redir_len;
    return n;
}

struct ast *ast_new_list(struct ast **items, size_t len)
{
    struct ast *n = calloc(1, sizeof(*n));
    if (!n) abort();
    n->type = AST_LIST;
    n->as.list.items = items;
    n->as.list.len = len;
    return n;
}

struct ast *ast_new_if(struct ast *cond, struct ast *then_branch,
                       struct ast **elif_conds, struct ast **elif_thens, size_t elif_len,
                       struct ast *else_branch)
{
    struct ast *n = calloc(1, sizeof(*n));
    if (!n) abort();
    n->type = AST_IF;
    n->as.ifnode.cond = cond;
    n->as.ifnode.then_branch = then_branch;
    n->as.ifnode.elif_conds = elif_conds;
    n->as.ifnode.elif_thens = elif_thens;
    n->as.ifnode.elif_len = elif_len;
    n->as.ifnode.else_branch = else_branch;
    return n;
}

void ast_free(struct ast *n)
{
    if (!n) return;

    if (n->type == AST_SIMPLE) {
        free_argv(n->as.simple.argv);
        free_redirs(n->as.simple.redirs, n->as.simple.redir_len);
    } else if (n->type == AST_LIST) {
        for (size_t i = 0; i < n->as.list.len; i++)
            ast_free(n->as.list.items[i]);
        free(n->as.list.items);
    } else if (n->type == AST_IF) {
        ast_free(n->as.ifnode.cond);
        ast_free(n->as.ifnode.then_branch);
        for (size_t i = 0; i < n->as.ifnode.elif_len; i++) {
            ast_free(n->as.ifnode.elif_conds[i]);
            ast_free(n->as.ifnode.elif_thens[i]);
        }
        free(n->as.ifnode.elif_conds);
        free(n->as.ifnode.elif_thens);
        ast_free(n->as.ifnode.else_branch);
    }

    free(n);
}
