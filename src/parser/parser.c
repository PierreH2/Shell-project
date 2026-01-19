#include "parser.h"
#include "util/error.h"
#include "util/vec.h"
#include <stdlib.h>
#include <string.h>

static int is_sep(enum token_type t)
{
    return t == TOK_SEMI || t == TOK_NL;
}

static int is_stop(enum token_type t, int stop_then, int stop_else, int stop_fi)
{
    if (stop_then && t == TOK_THEN) return 1;
    if (stop_else && (t == TOK_ELIF || t == TOK_ELSE)) return 1;
    if (stop_fi && t == TOK_FI) return 1;
    return 0;
}

static void expect(struct lexer *lx, enum token_type type, const char *msg)
{
    struct token t = lexer_next(lx);
    if (t.type != type) {
        int line = t.line, col = t.col;
        token_free(&t);
        syntax_error(line, col, msg);
    }
    token_free(&t);
}

static struct ast *parse_command(struct lexer *lx);

static struct ast *parse_simple_command(struct lexer *lx, struct token first)
{
    struct vec args;
    vec_init(&args);

    if (first.type != TOK_WORD) {
        int line = first.line, col = first.col;
        token_free(&first);
        syntax_error(line, col, "expected WORD");
    }
    vec_push(&args, first.value); // take ownership
    first.value = NULL;
    token_free(&first);

    while (1) {
        struct token t = lexer_peek(lx);
        if (t.type != TOK_WORD)
            break;

        t = lexer_next(lx);
        vec_push(&args, t.value);
        t.value = NULL;
        token_free(&t);
    }

    // build argv null-terminated
    char **argv = calloc(args.len + 1, sizeof(char *));
    if (!argv) abort();
    for (size_t i = 0; i < args.len; i++)
        argv[i] = (char *)vec_get(&args, i);
    argv[args.len] = NULL;

    vec_free(&args);
    return ast_new_simple(argv);
}

static struct ast *parse_compound_list(struct lexer *lx,
                                       int stop_then, int stop_else, int stop_fi)
{
    // skip leading separators/newlines
    while (1) {
        struct token p = lexer_peek(lx);
        if (is_sep(p.type)) {
            p = lexer_next(lx);
            token_free(&p);
            continue;
        }
        break;
    }

    struct vec items;
    vec_init(&items);

    while (1) {
        struct token p = lexer_peek(lx);

        if (p.type == TOK_EOF || is_stop(p.type, stop_then, stop_else, stop_fi))
            break;

        struct ast *cmd = parse_command(lx);
        vec_push(&items, cmd);

        // consume any separators
        while (1) {
            struct token s = lexer_peek(lx);
            if (is_sep(s.type)) {
                s = lexer_next(lx);
                token_free(&s);
                continue;
            }
            break;
        }
    }

    // empty compound_list is allowed in some places? for Step1 keep it error
    if (items.len == 0) {
        // if we stopped on a stopper token, empty list is syntax error
        struct token t = lexer_peek(lx);
        syntax_error(t.line, t.col, "expected command");
    }

    struct ast **arr = calloc(items.len, sizeof(struct ast *));
    if (!arr) abort();
    for (size_t i = 0; i < items.len; i++)
        arr[i] = (struct ast *)vec_get(&items, i);

    size_t len = items.len;
    vec_free(&items);
    return ast_new_list(arr, len);
}

static struct ast *parse_if(struct lexer *lx)
{
    // consume IF already peeked by parse_command
    struct token tif = lexer_next(lx);
    int if_line = tif.line, if_col = tif.col;
    token_free(&tif);

    struct ast *cond = parse_compound_list(lx, 1, 0, 0); // stop on THEN
    expect(lx, TOK_THEN, "expected 'then'");

    struct ast *then_branch = parse_compound_list(lx, 0, 1, 1); // stop on ELIF/ELSE/FI

    // elif*
    struct vec elif_conds;
    struct vec elif_thens;
    vec_init(&elif_conds);
    vec_init(&elif_thens);

    while (1) {
        struct token p = lexer_peek(lx);
        if (p.type != TOK_ELIF)
            break;

        p = lexer_next(lx);
        token_free(&p);

        struct ast *ec = parse_compound_list(lx, 1, 0, 0); // stop on THEN
        expect(lx, TOK_THEN, "expected 'then' after elif condition");
        struct ast *et = parse_compound_list(lx, 0, 1, 1); // stop on ELIF/ELSE/FI

        vec_push(&elif_conds, ec);
        vec_push(&elif_thens, et);
    }

    // else?
    struct ast *else_branch = NULL;
    struct token p = lexer_peek(lx);
    if (p.type == TOK_ELSE) {
        p = lexer_next(lx);
        token_free(&p);
        else_branch = parse_compound_list(lx, 0, 0, 1); // stop on FI
    }

    // fi
    struct token end = lexer_next(lx);
    if (end.type != TOK_FI) {
        token_free(&end);
        syntax_error(if_line, if_col, "expected 'fi'");
    }
    token_free(&end);

    // build elif arrays
    struct ast **ec_arr = NULL;
    struct ast **et_arr = NULL;
    size_t n = elif_conds.len;

    if (n > 0) {
        ec_arr = calloc(n, sizeof(struct ast *));
        et_arr = calloc(n, sizeof(struct ast *));
        if (!ec_arr || !et_arr) abort();
        for (size_t i = 0; i < n; i++) {
            ec_arr[i] = (struct ast *)vec_get(&elif_conds, i);
            et_arr[i] = (struct ast *)vec_get(&elif_thens, i);
        }
    }

    vec_free(&elif_conds);
    vec_free(&elif_thens);

    return ast_new_if(cond, then_branch, ec_arr, et_arr, n, else_branch);
}

static struct ast *parse_command(struct lexer *lx)
{
    struct token p = lexer_peek(lx);

    if (p.type == TOK_IF) {
        return parse_if(lx);
    }

    if (p.type == TOK_WORD) {
        p = lexer_next(lx);
        return parse_simple_command(lx, p);
    }

    syntax_error(p.line, p.col, "unexpected token, expected command");
    return NULL;
}

struct ast *parse_input(struct lexer *lx)
{
    struct token p = lexer_peek(lx);
    if (p.type == TOK_EOF)
        return NULL;

    // root: compound_list until EOF
    struct ast *root = parse_compound_list(lx, 0, 0, 0);

    // allow trailing separators/newlines
    while (1) {
        struct token t = lexer_peek(lx);
        if (is_sep(t.type)) {
            t = lexer_next(lx);
            token_free(&t);
            continue;
        }
        break;
    }

    p = lexer_next(lx);
    if (p.type != TOK_EOF) {
        int line = p.line, col = p.col;
        token_free(&p);
        syntax_error(line, col, "expected end of input");
    }
    token_free(&p);
    return root;
}
