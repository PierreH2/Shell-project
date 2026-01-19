#ifndef AST_H
#define AST_H

#include <stddef.h>

enum ast_type {
    AST_SIMPLE,
    AST_LIST,
    AST_IF
};

struct ast;

struct ast_list {
    struct ast **items;
    size_t len;
};

struct ast_simple {
    char **argv; // NULL-terminated
};

struct ast_if {
    struct ast *cond;
    struct ast *then_branch;

    struct ast **elif_conds;
    struct ast **elif_thens;
    size_t elif_len;

    struct ast *else_branch; // optional
};

struct ast {
    enum ast_type type;
    union {
        struct ast_simple simple;
        struct ast_list list;
        struct ast_if ifnode;
    } as;
};

struct ast *ast_new_simple(char **argv);
struct ast *ast_new_list(struct ast **items, size_t len);
struct ast *ast_new_if(struct ast *cond, struct ast *then_branch,
                       struct ast **elif_conds, struct ast **elif_thens, size_t elif_len,
                       struct ast *else_branch);

void ast_free(struct ast *n);

#endif
