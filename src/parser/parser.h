#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "ast.h"

struct ast *parse_input(struct lexer *lx);

#endif
