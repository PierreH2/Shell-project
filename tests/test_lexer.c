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
// Redirections
Test(lexer_redirections, output_redirection)
{
    struct lexer lx = make_lexer("echo hello > file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);
    struct token t4 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_WORD);
    cr_assert_eq(t3.type, TOK_REDIR_OUT);
    cr_assert_eq(t4.type, TOK_WORD);
    cr_assert_str_eq(t4.value, "file.txt");
}

Test(lexer_redirections, input_redirection)
{
    struct lexer lx = make_lexer("cat < input.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_REDIR_IN);
    cr_assert_eq(t3.type, TOK_WORD);
}

Test(lexer_redirections, append_redirection)
{
    struct lexer lx = make_lexer("echo data >> file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);
    struct token t4 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_WORD);  // "data"
    cr_assert_eq(t3.type, TOK_REDIR_APPEND);
    cr_assert_eq(t4.type, TOK_WORD);
}

Test(lexer_redirections, clobber_redirection)
{
    struct lexer lx = make_lexer("echo data >| file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);
    struct token t4 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_WORD);  // "data"
    cr_assert_eq(t3.type, TOK_REDIR_CLOBBER);
    cr_assert_eq(t4.type, TOK_WORD);
}

Test(lexer_redirections, rdwr_redirection)
{
    struct lexer lx = make_lexer("command <> file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_REDIR_RDWR);
    cr_assert_eq(t3.type, TOK_WORD);
}

Test(lexer_redirections, out_err_redirection)
{
    struct lexer lx = make_lexer("cmd >& file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_REDIR_OUT_ERR);
    cr_assert_eq(t3.type, TOK_WORD);
}

Test(lexer_redirections, in_err_redirection)
{
    struct lexer lx = make_lexer("cmd <& fd");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_REDIR_IN_ERR);
    cr_assert_eq(t3.type, TOK_WORD);
}

Test(lexer_redirections, ionumber_token)
{
    struct lexer lx = make_lexer("echo 2> error.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_IONUMBER);
    cr_assert_str_eq(t2.value, "2");
    cr_assert_eq(t3.type, TOK_REDIR_OUT);
}

Test(lexer_redirections, multiple_ionumber_digits)
{
    struct lexer lx = make_lexer("echo 123> file.txt");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_IONUMBER);
    cr_assert_str_eq(t2.value, "123");
    cr_assert_eq(t3.type, TOK_REDIR_OUT);
}

Test(lexer_redirections, number_without_redir_is_word)
{
    struct lexer lx = make_lexer("echo 42 hello");

    struct token t1 = lexer_next(&lx);
    struct token t2 = lexer_next(&lx);
    struct token t3 = lexer_next(&lx);

    cr_assert_eq(t1.type, TOK_WORD);
    cr_assert_eq(t2.type, TOK_WORD);
    cr_assert_str_eq(t2.value, "42");
    cr_assert_eq(t3.type, TOK_WORD);
}