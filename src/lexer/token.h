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
