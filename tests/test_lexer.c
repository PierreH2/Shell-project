#include <criterion/criterion.h>
#include <string.h>
#include <stdio.h>

#include "lexer/lexer.h"

// helper function
static struct lexer make_lexer(const char *input)
{
    FILE *f = fmemopen((void *)input, strlen(input), "r");
    cr_assert_not_null(f);

    struct lexer lx;
    lexer_init(&lx, f);
    return lx;
}

// Core
Test(lexer_core, simple_word)
{
    struct lexer lx = make_lexer("echo");

    struct token tok = lexer_next(&lx);

    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "echo");
}

// whitespaces
Test(lexer_whitespace, skips_leading_spaces)
{
    struct lexer lx = make_lexer("   \t  echo");

    struct token tok = lexer_next(&lx);

    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "echo");
}

Test(lexer_whitespace, multiple_spaces_between_words)
{
    struct lexer lx = make_lexer("echo    hello");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);

    cr_assert_str_eq(t1.value, "echo");
    cr_assert_str_eq(t2.value, "hello");
}

// quotes
Test(lexer_quotes, single_quotes_are_single_token)
{
    struct lexer lx = make_lexer("echo 'hello world'");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);

    cr_assert_str_eq(t1.value, "echo");
    cr_assert_eq(t2.type, TOK_WORD);
    cr_assert_str_eq(t2.value, "hello world");
}

// EOF handling
Test(lexer_eof, empty_input)
{
    struct lexer lx = make_lexer("");

    struct token tok = lexer_next(&lx);

    cr_assert_eq(tok.type, TOK_EOF);
}

Test(lexer_eof, only_spaces)
{
    struct lexer lx = make_lexer("   \t   ");

    struct token tok = lexer_next(&lx);

    cr_assert_eq(tok.type, TOK_EOF);
}
