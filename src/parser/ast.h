#ifndef AST_H
#define AST_H

#include <stddef.h>

enum ast_type {
    AST_SIMPLE,
    AST_LIST,
    AST_IF
};

enum redir_type {
    REDIR_IN,       /* < */
    REDIR_OUT,      /* > */
    REDIR_APPEND,   /* >> */
    REDIR_CLOBBER,  /* >| */
    REDIR_OUT_ERR,  /* >& */
    REDIR_IN_ERR,   /* <& */
    REDIR_RDWR      /* <> */
};

struct redirection {
    enum redir_type type;
    char *target;           /* filename or target fd */
    int fd;                 /* file descriptor (default: 1 for out, 0 for in, -1 if not specified) */
};

struct ast;

struct ast_list {
    struct ast **items;
    size_t len;
};

struct ast_simple {
    char **argv; // NULL-terminated
    struct redirection *redirs;
    size_t redir_len;
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
struct ast *ast_new_simple_with_redirs(char **argv, struct redirection *redirs, size_t redir_len);
struct ast *ast_new_list(struct ast **items, size_t len);
struct ast *ast_new_if(struct ast *cond, struct ast *then_branch,
                       struct ast **elif_conds, struct ast **elif_thens, size_t elif_len,
                       struct ast *else_branch);

void ast_free(struct ast *n);

#endif
