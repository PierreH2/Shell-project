#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"

struct lexer {
    FILE *in;
    int line;
    int col;
    int at_cmd_start;     // pour reconnaître les mots réservés
    int has_peek;
    struct token peeked;
};

void lexer_init(struct lexer *lx, FILE *in);
struct token lexer_peek(struct lexer *lx);
struct token lexer_next(struct lexer *lx);

#endif
