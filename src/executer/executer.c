#include "executer.h"
#include "builtins.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int exec_simple(char **argv)
{
    int st = 0;
    if (try_builtin(argv, &st))
        return st;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(wstatus))
        return WEXITSTATUS(wstatus);
    if (WIFSIGNALED(wstatus))
        return 128 + WTERMSIG(wstatus);

    return 1;
}

static int exec_list(struct ast **items, size_t len)
{
    int st = 0;
    for (size_t i = 0; i < len; i++)
        st = exec_ast(items[i]);
    return st;
}

static int exec_if(struct ast_if *ifn)
{
    int cond = exec_ast(ifn->cond);
    if (cond == 0)
        return exec_ast(ifn->then_branch);

    for (size_t i = 0; i < ifn->elif_len; i++) {
        int c = exec_ast(ifn->elif_conds[i]);
        if (c == 0)
            return exec_ast(ifn->elif_thens[i]);
    }

    if (ifn->else_branch)
        return exec_ast(ifn->else_branch);

    return 0;
}

int exec_ast(struct ast *n)
{
    if (!n)
        return 0;

    if (n->type == AST_SIMPLE)
        return exec_simple(n->as.simple.argv);

    if (n->type == AST_LIST)
        return exec_list(n->as.list.items, n->as.list.len);

    if (n->type == AST_IF)
        return exec_if(&n->as.ifnode);

    return 1;
}
