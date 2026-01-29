#include "lexer.h"
#include "util/str.h"
#include "util/error.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static struct token make_tok(struct lexer *lx, enum token_type type, char *val, int line, int col)
{
    struct token t;
    t.type = type;
    t.value = val;
    t.line = line;
    t.col = col;
    return t;
}

void token_free(struct token *t)
{
    if (t->type == TOK_WORD)
        free(t->value);
    t->value = NULL;
}

static int is_word_break(int c)
{
    return c == EOF || c == ';' || c == '\n' || c == ' ' || c == '\t'
           || c == '<' || c == '>';
}

static int lx_getc(struct lexer *lx)
{
    int c = fgetc(lx->in);
    if (c == '\n') {
        lx->line++;
        lx->col = 0;
    } else if (c != EOF) {
        lx->col++;
    }
    return c;
}

static void lx_ungetc(struct lexer *lx, int c)
{
    if (c == EOF)
        return;
    ungetc(c, lx->in);
    if (c == '\n') {
        // approximate (rarely used)
        lx->line--;
        lx->col = 0;
    } else {
        lx->col--;
    }
}

static enum token_type reserved_type(const char *w)
{
    if (strcmp(w, "if") == 0) return TOK_IF;
    if (strcmp(w, "then") == 0) return TOK_THEN;
    if (strcmp(w, "elif") == 0) return TOK_ELIF;
    if (strcmp(w, "else") == 0) return TOK_ELSE;
    if (strcmp(w, "fi") == 0) return TOK_FI;
    return TOK_WORD;
}

static void skip_spaces(struct lexer *lx)
{
    int c;
    while (1) {
        c = lx_getc(lx);
        if (c == ' ' || c == '\t')
            continue;
        lx_ungetc(lx, c);
        return;
    }
}

static int skip_comment_if_any(struct lexer *lx)
{
    // comment starts only if '#' is first char of a word AND we're at cmd start
    int c = lx_getc(lx);
    if (c != '#') {
        lx_ungetc(lx, c);
        return 0;
    }

    // consume until newline or EOF
    while (1) {
        c = lx_getc(lx);
        if (c == EOF)
            return 1;
        if (c == '\n') {
            return 2;
        }
    }
}

static void read_single_quotes(struct lexer *lx, struct str *sb, int start_line, int start_col)
{
    // we have already consumed the opening quote
    int c;
    while (1) {
        c = lx_getc(lx);
        if (c == EOF)
            syntax_error(start_line, start_col, "unterminated single quote");
        if (c == '\'')
            return;
        str_pushc(sb, (char)c);
    }
}

static struct token lex_redir_or_ionumber(struct lexer *lx)
{
    int c = lx_getc(lx);
    int line = lx->line;
    int col = lx->col;

    /* Check for IO number: [0-9]+ followed by a redirection operator */
    if (isdigit(c)) {
        struct str num;
        str_init(&num);
        str_pushc(&num, (char)c);

        int next;
        while (1) {
            next = lx_getc(lx);
            if (isdigit(next)) {
                str_pushc(&num, (char)next);
            } else {
                lx_ungetc(lx, next);
                break;
            }
        }

        /* Check if next char is a redirection operator */
        next = lx_getc(lx);
        if (next == '<' || next == '>') {
            /* This is an IO number */
            lx_ungetc(lx, next);
            char *ionum = str_take(&num);
            str_free(&num);
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_IONUMBER, ionum, line, col);
        } else {
            /* Not an IO number, treat as word */
            lx_ungetc(lx, next);
            char *word = str_take(&num);
            str_free(&num);
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_WORD, word, line, col);
        }
    } else if (c == '<') {
        /* < or <& or <> */
        int next = lx_getc(lx);
        if (next == '&') {
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_IN_ERR, NULL, line, col);
        } else if (next == '>') {
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_RDWR, NULL, line, col);
        } else {
            lx_ungetc(lx, next);
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_IN, NULL, line, col);
        }
    } else if (c == '>') {
        /* > or >> or >& or >| */
        int next = lx_getc(lx);
        if (next == '>') {
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_APPEND, NULL, line, col);
        } else if (next == '&') {
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_OUT_ERR, NULL, line, col);
        } else if (next == '|') {
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_CLOBBER, NULL, line, col);
        } else {
            lx_ungetc(lx, next);
            lx->at_cmd_start = 0;
            return make_tok(lx, TOK_REDIR_OUT, NULL, line, col);
        }
    }

    lx_ungetc(lx, c);
    syntax_error(line, col, "invalid redirection operator");
    return make_tok(lx, TOK_EOF, NULL, line, col);
}

static struct token lex_word(struct lexer *lx)
{
    int start_line = lx->line;
    int start_col = lx->col;

    struct str sb;
    str_init(&sb);

    while (1) {
        int c = lx_getc(lx);
        if (c == EOF) {
            break;
        }
        if (is_word_break(c)) {
            lx_ungetc(lx, c);
            break;
        }
        if (c == '#') {
            // '#' inside a word is literal
            str_pushc(&sb, '#');
            continue;
        }
        if (c == '\'') {
            read_single_quotes(lx, &sb, start_line, start_col);
            continue;
        }
        str_pushc(&sb, (char)c);
    }

    char *w = str_take(&sb);
    str_free(&sb);

    if (lx->at_cmd_start) {
        enum token_type rt = reserved_type(w);
        if (rt != TOK_WORD) {
            free(w);
            lx->at_cmd_start = 1; // still at command start for following compound_list
            return make_tok(lx, rt, NULL, start_line, start_col);
        }
    }

    lx->at_cmd_start = 0;
    return make_tok(lx, TOK_WORD, w, start_line, start_col);
}

static struct token lex_one(struct lexer *lx)
{
    int comment_res;

    /* 1. Skip whitespace AND comments as long as they appear */
    while (1) {
        skip_spaces(lx);

        comment_res = skip_comment_if_any(lx);
        if (comment_res == 0)
            break;

        if (comment_res == 2) {
            return make_tok(lx, TOK_NL, NULL, lx->line, lx->col);
        }
        /* comment without newline → continue */
    }

    int c = lx_getc(lx);
    int line = lx->line;
    int col = lx->col;

    if (c == EOF)
        return make_tok(lx, TOK_EOF, NULL, line, col);

    if (c == ';') {
        lx->at_cmd_start = 1;
        return make_tok(lx, TOK_SEMI, NULL, line, col);
    }

    if (c == '\n') {
        lx->at_cmd_start = 1;
        return make_tok(lx, TOK_NL, NULL, line, col);
    }

    /* Check for redirections or IO numbers */
    if (c == '<' || c == '>' || isdigit(c)) {
        lx_ungetc(lx, c);
        return lex_redir_or_ionumber(lx);
    }

    lx_ungetc(lx, c);
    return lex_word(lx);
}

void lexer_init(struct lexer *lx, FILE *in)
{
    lx->in = in;
    lx->line = 1;
    lx->col = 0;
    lx->at_cmd_start = 1;
    lx->has_peek = 0;
}

struct token lexer_peek(struct lexer *lx)
{
    if (!lx->has_peek) {
        lx->peeked = lex_one(lx);
        lx->has_peek = 1;
    }
    return lx->peeked;
}

struct token lexer_next(struct lexer *lx)
{
    if (lx->has_peek) {
        lx->has_peek = 0;
        return lx->peeked;
    }
    return lex_one(lx);
}
