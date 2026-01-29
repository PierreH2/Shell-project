#include "parser.h"
#include "util/error.h"
#include "util/vec.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static int is_redir_token(enum token_type t)
{
    return t == TOK_REDIR_IN || t == TOK_REDIR_OUT || t == TOK_REDIR_APPEND
        || t == TOK_REDIR_CLOBBER || t == TOK_REDIR_OUT_ERR || t == TOK_REDIR_IN_ERR
        || t == TOK_REDIR_RDWR;
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

static enum redir_type token_to_redir_type(enum token_type t)
{
    switch (t) {
        case TOK_REDIR_IN:      return REDIR_IN;
        case TOK_REDIR_OUT:     return REDIR_OUT;
        case TOK_REDIR_APPEND:  return REDIR_APPEND;
        case TOK_REDIR_CLOBBER: return REDIR_CLOBBER;
        case TOK_REDIR_OUT_ERR: return REDIR_OUT_ERR;
        case TOK_REDIR_IN_ERR:  return REDIR_IN_ERR;
        case TOK_REDIR_RDWR:    return REDIR_RDWR;
        default:                return REDIR_IN; /* shouldn't happen */
    }
}

static int default_fd_for_redir(enum redir_type type)
{
    switch (type) {
        case REDIR_IN:
        case REDIR_IN_ERR:
        case REDIR_RDWR:
            return 0;   /* stdin */
        default:
            return 1;   /* stdout */
    }
}

static struct ast *parse_simple_command(struct lexer *lx, struct token first)
{
    struct vec args;
    struct vec redirs;
    vec_init(&args);
    vec_init(&redirs);

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
        
        if (t.type == TOK_WORD) {
            t = lexer_next(lx);
            vec_push(&args, t.value);
            t.value = NULL;
            token_free(&t);
        } else if (t.type == TOK_IONUMBER) {
            /* IONumber redirect */
            t = lexer_next(lx);
            int ionum = atoi(t.value);
            token_free(&t);

            struct token redir_tok = lexer_next(lx);
            if (!is_redir_token(redir_tok.type)) {
                int line = redir_tok.line, col = redir_tok.col;
                token_free(&redir_tok);
                syntax_error(line, col, "expected redirection operator");
            }

            struct token target_tok = lexer_next(lx);
            if (target_tok.type != TOK_WORD) {
                int line = target_tok.line, col = target_tok.col;
                token_free(&target_tok);
                syntax_error(line, col, "expected redirection target");
            }

            struct redirection *r = calloc(1, sizeof(struct redirection));
            if (!r) abort();
            r->type = token_to_redir_type(redir_tok.type);
            r->target = target_tok.value;
            r->fd = ionum;
            vec_push(&redirs, r);

            token_free(&redir_tok);
        } else if (is_redir_token(t.type)) {
            /* Simple redirect without IONumber */
            t = lexer_next(lx);
            enum redir_type rtype = token_to_redir_type(t.type);
            token_free(&t);

            struct token target_tok = lexer_next(lx);
            if (target_tok.type != TOK_WORD) {
                int line = target_tok.line, col = target_tok.col;
                token_free(&target_tok);
                syntax_error(line, col, "expected redirection target");
            }

            struct redirection *r = calloc(1, sizeof(struct redirection));
            if (!r) abort();
            r->type = rtype;
            r->target = target_tok.value;
            r->fd = default_fd_for_redir(rtype);
            vec_push(&redirs, r);
        } else {
            break;
        }
    }

    // build argv null-terminated
    char **argv = calloc(args.len + 1, sizeof(char *));
    if (!argv) abort();
    for (size_t i = 0; i < args.len; i++)
        argv[i] = (char *)vec_get(&args, i);
    argv[args.len] = NULL;

    // build redirs array
    struct redirection *redirs_arr = NULL;
    size_t redirs_len = 0;
    if (redirs.len > 0) {
        redirs_arr = calloc(redirs.len, sizeof(struct redirection));
        if (!redirs_arr) abort();
        for (size_t i = 0; i < redirs.len; i++) {
            struct redirection *src = (struct redirection *)vec_get(&redirs, i);
            redirs_arr[i] = *src;
            free(src);  // Free the heap-allocated redirections from vector
        }
        redirs_len = redirs.len;
    }

    vec_free(&args);
    vec_free(&redirs);

    if (redirs_len > 0)
        return ast_new_simple_with_redirs(argv, redirs_arr, redirs_len);
    else
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
