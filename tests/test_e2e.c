#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "executer/executer.h"

static int run_script(const char *s)
{
    FILE *f = fmemopen((void *)s, strlen(s), "r");
    cr_assert_not_null(f);

    struct lexer lx;
    lexer_init(&lx, f);

    struct ast *root = parse_input(&lx);
    int st = exec_ast(root);

    ast_free(root);
    fclose(f);
    return st;
}

static void redirect_all(void)
{
    cr_redirect_stdout();
    cr_redirect_stderr();
}

Test(e2e, echo_hello, .init = redirect_all)
{
    int st = run_script("echo hello");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello\n");
}

Test(e2e, multiple_commands, .init = redirect_all)
{
    int st = run_script("echo a; echo b");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("a\nb\n");
}

Test(e2e, if_else, .init = redirect_all)
{
    int st = run_script("if false; then echo no; else echo yes; fi");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("yes\n");
}

Test(e2e, elif_chain, .init = redirect_all)
{
    int st = run_script(
        "if false; then echo no;"
        "elif true; then echo ok;"
        "else echo bad; fi");

    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("ok\n");
}

Test(e2e, quotes_and_comments, .init = redirect_all)
{
    int st = run_script(
        "echo 'hello world' # comment\n"
        "echo ok\n");   

    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello world\nok\n");
}

// Redirection tests
Test(e2e, output_redirection_to_file)
{
    char tmpfile[] = "/tmp/test_e2e_redir_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    char script[256];
    snprintf(script, sizeof(script), "echo 'test content' > %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    // Verify file contents
    FILE *f = fopen(tmpfile, "r");
    cr_assert_not_null(f);
    char buf[100];
    fgets(buf, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "test content") != NULL);
    unlink(tmpfile);
}

Test(e2e, append_redirection)
{
    char tmpfile[] = "/tmp/test_e2e_append_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    // Write initial content
    FILE *f = fopen(tmpfile, "w");
    fprintf(f, "initial\n");
    fclose(f);

    char script[256];
    snprintf(script, sizeof(script), "echo 'appended' >> %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    // Verify both lines exist
    f = fopen(tmpfile, "r");
    char buf[200];
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "initial") != NULL);
    cr_assert(strstr(buf, "appended") != NULL);
    unlink(tmpfile);
}

Test(e2e, input_redirection)
{
    char tmpfile[] = "/tmp/test_e2e_input_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    // Write test content
    FILE *f = fopen(tmpfile, "w");
    fprintf(f, "input data\n");
    fclose(f);

    char script[256];
    snprintf(script, sizeof(script), "cat < %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    unlink(tmpfile);
}

Test(e2e, multiple_redirections)
{
    char out[] = "/tmp/test_e2e_out_XXXXXX";
    char err[] = "/tmp/test_e2e_err_XXXXXX";
    int fd1 = mkstemp(out);
    int fd2 = mkstemp(err);
    close(fd1);
    close(fd2);

    char script[256];
    snprintf(script, sizeof(script), "echo 'output' > %s; echo 'also output' > %s", out, err);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    // Check output file
    FILE *f = fopen(out, "r");
    char buf[100];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    cr_assert(strstr(buf, "output") != NULL);

    // Check second file
    f = fopen(err, "r");
    fgets(buf, sizeof(buf), f);
    fclose(f);
    cr_assert(strstr(buf, "also output") != NULL);

    unlink(out);
    unlink(err);
}

Test(e2e, clobber_redirection)
{
    char tmpfile[] = "/tmp/test_e2e_clobber_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    // Write initial content
    FILE *f = fopen(tmpfile, "w");
    fprintf(f, "initial content\n");
    fclose(f);

    char script[256];
    snprintf(script, sizeof(script), "echo 'clobbered' >| %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    // Verify file was overwritten
    f = fopen(tmpfile, "r");
    char buf[100];
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "clobbered") != NULL);
    cr_assert(strstr(buf, "initial") == NULL); // old content should be gone

    unlink(tmpfile);
}

Test(e2e, ionumber_redirection)
{
    char tmpfile[] = "/tmp/test_e2e_ionumber_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    char script[256];
    snprintf(script, sizeof(script), "echo 'output via fd1' 1> %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    // Check output file
    FILE *f = fopen(tmpfile, "r");
    char buf[100];
    fgets(buf, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "output via fd1") != NULL);
    unlink(tmpfile);
}

// Pipeline tests
Test(e2e, simple_pipeline, .init = redirect_all)
{
    int st = run_script("echo hello | cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello\n");
}

Test(e2e, pipeline_exit_status_true_false)
{
    /* Exit status should be from last command: true | false exits 1 */
    int st = run_script("true | false");
    cr_assert_eq(st, 1);
}

Test(e2e, pipeline_exit_status_false_true)
{
    /* Exit status should be from last command: false | true exits 0 */
    int st = run_script("false | true");
    cr_assert_eq(st, 0);
}

Test(e2e, three_command_pipeline, .init = redirect_all)
{
    int st = run_script("echo 'line1\nline2\nline3' | cat | cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("line1\nline2\nline3\n");
}

Test(e2e, pipeline_with_grep, .init = redirect_all)
{
    /* Create a test file to grep through */
    char tmpfile[] = "/tmp/test_e2e_grep_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *f = fdopen(fd, "w");
    fprintf(f, "apple\nbanana\ncherry\n");
    fclose(f);

    char script[256];
    snprintf(script, sizeof(script), "cat %s | grep banana", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("banana\n");
    
    unlink(tmpfile);
}

Test(e2e, pipeline_with_wc, .init = redirect_all)
{
    int st = run_script("echo -e 'a\nb\nc' | wc -l");
    cr_assert_eq(st, 0);
    /* wc -l should count 3 lines */
}

Test(e2e, long_pipeline, .init = redirect_all)
{
    /* Test a pipeline with many commands */
    int st = run_script("echo test | cat | cat | cat | cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("test\n");
}

Test(e2e, pipeline_with_input_redirection, .init = redirect_all)
{
    /* Create a test file */
    char tmpfile[] = "/tmp/test_e2e_pipe_in_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *f = fdopen(fd, "w");
    fprintf(f, "file content\n");
    fclose(f);

    char script[256];
    snprintf(script, sizeof(script), "cat < %s | cat", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("file content\n");
    
    unlink(tmpfile);
}

Test(e2e, pipeline_with_output_redirection)
{
    /* Redirect output of pipeline to a file */
    char tmpfile[] = "/tmp/test_e2e_pipe_out_XXXXXX";
    int fd = mkstemp(tmpfile);
    close(fd);

    char script[256];
    snprintf(script, sizeof(script), "echo pipeline | cat > %s", tmpfile);
    
    int st = run_script(script);
    cr_assert_eq(st, 0);

    /* Verify file contents */
    FILE *f = fopen(tmpfile, "r");
    char buf[100];
    fgets(buf, sizeof(buf), f);
    fclose(f);

    cr_assert(strstr(buf, "pipeline") != NULL);
    unlink(tmpfile);
}

Test(e2e, pipeline_with_newlines, .init = redirect_all)
{
    /* Test pipeline with newlines after pipe symbol */
    int st = run_script("echo test |\n cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("test\n");
}

Test(e2e, multiple_pipelines, .init = redirect_all)
{
    /* Multiple pipelines in a command list */
    int st = run_script("echo a | cat; echo b | cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("a\nb\n");
}

Test(e2e, pipeline_in_if_condition, .init = redirect_all)
{
    /* Use pipeline in if condition */
    int st = run_script("if true | true; then echo ok; fi");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("ok\n");
}

Test(e2e, pipeline_builtin_true_false)
{
    /* Test with builtin commands */
    int st = run_script("true | false | true");
    cr_assert_eq(st, 0); /* Last command is true */
}

Test(e2e, pipeline_builtin_echo, .init = redirect_all)
{
    /* Test builtin echo in pipeline */
    int st = run_script("echo hello | cat");
    cr_assert_eq(st, 0);
    cr_assert_stdout_eq_str("hello\n");
}
