#ifndef TOKEN_H
#define TOKEN_H

enum token_type {
    TOK_WORD,
    TOK_IF,
    TOK_THEN,
    TOK_ELIF,
    TOK_ELSE,
    TOK_FI,
    TOK_SEMI,
    TOK_NL,
    /* Redirections */
    TOK_REDIR_IN,       /* < */
    TOK_REDIR_OUT,      /* > */
    TOK_REDIR_APPEND,   /* >> */
    TOK_REDIR_CLOBBER,  /* >| */
    TOK_REDIR_OUT_ERR,  /* >& */
    TOK_REDIR_IN_ERR,   /* <& */
    TOK_REDIR_RDWR,     /* <> */
    TOK_IONUMBER,       /* [0-9]+ for file descriptor */
    TOK_EOF
};

struct token {
    enum token_type type;
    char *value;   // only for TOK_WORD
    int line;
    int col;
};

void token_free(struct token *t);

#endif
