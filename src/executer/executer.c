#include "executer.h"
#include "builtins.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

static int apply_redirections(struct redirection *redirs, size_t redir_len)
{
    for (size_t i = 0; i < redir_len; i++) {
        struct redirection *r = &redirs[i];
        int fd = r->fd;
        int mode = 0644;
        int target_fd = -1;

        switch (r->type) {
            case REDIR_IN:
                /* < file: read from file */
                target_fd = open(r->target, O_RDONLY);
                if (target_fd < 0) {
                    perror(r->target);
                    return -1;
                }
                if (dup2(target_fd, fd) < 0) {
                    perror("dup2");
                    close(target_fd);
                    return -1;
                }
                close(target_fd);
                break;

            case REDIR_OUT:
                /* > file: write to file (truncate) */
                target_fd = open(r->target, O_WRONLY | O_CREAT | O_TRUNC, mode);
                if (target_fd < 0) {
                    perror(r->target);
                    return -1;
                }
                if (dup2(target_fd, fd) < 0) {
                    perror("dup2");
                    close(target_fd);
                    return -1;
                }
                close(target_fd);
                break;

            case REDIR_APPEND:
                /* >> file: append to file */
                target_fd = open(r->target, O_WRONLY | O_CREAT | O_APPEND, mode);
                if (target_fd < 0) {
                    perror(r->target);
                    return -1;
                }
                if (dup2(target_fd, fd) < 0) {
                    perror("dup2");
                    close(target_fd);
                    return -1;
                }
                close(target_fd);
                break;

            case REDIR_CLOBBER:
                /* >| file: write to file, clobber (same as > for our purposes) */
                target_fd = open(r->target, O_WRONLY | O_CREAT | O_TRUNC, mode);
                if (target_fd < 0) {
                    perror(r->target);
                    return -1;
                }
                if (dup2(target_fd, fd) < 0) {
                    perror("dup2");
                    close(target_fd);
                    return -1;
                }
                close(target_fd);
                break;

            case REDIR_OUT_ERR:
                /* >& fd_or_file: redirect stdout to fd or file */
                /* Try to parse as fd number first */
                if (strcmp(r->target, "-") == 0) {
                    /* Special case: close fd */
                    close(fd);
                } else if (r->target[0] >= '0' && r->target[0] <= '9') {
                    int target = atoi(r->target);
                    if (dup2(target, fd) < 0) {
                        perror("dup2");
                        return -1;
                    }
                } else {
                    /* Treat as filename */
                    target_fd = open(r->target, O_WRONLY | O_CREAT | O_TRUNC, mode);
                    if (target_fd < 0) {
                        perror(r->target);
                        return -1;
                    }
                    if (dup2(target_fd, fd) < 0) {
                        perror("dup2");
                        close(target_fd);
                        return -1;
                    }
                    close(target_fd);
                }
                break;

            case REDIR_IN_ERR:
                /* <& fd_or_file: redirect stdin from fd or file */
                if (strcmp(r->target, "-") == 0) {
                    close(fd);
                } else if (r->target[0] >= '0' && r->target[0] <= '9') {
                    int target = atoi(r->target);
                    if (dup2(target, fd) < 0) {
                        perror("dup2");
                        return -1;
                    }
                } else {
                    target_fd = open(r->target, O_RDONLY);
                    if (target_fd < 0) {
                        perror(r->target);
                        return -1;
                    }
                    if (dup2(target_fd, fd) < 0) {
                        perror("dup2");
                        close(target_fd);
                        return -1;
                    }
                    close(target_fd);
                }
                break;

            case REDIR_RDWR:
                /* <> file: open file for both reading and writing */
                target_fd = open(r->target, O_RDWR | O_CREAT, mode);
                if (target_fd < 0) {
                    perror(r->target);
                    return -1;
                }
                if (dup2(target_fd, fd) < 0) {
                    perror("dup2");
                    close(target_fd);
                    return -1;
                }
                close(target_fd);
                break;
        }
    }
    return 0;
}

static int exec_simple(struct ast_simple *simple)
{
    char **argv = simple->argv;
    int st = 0;

    /* If this is a builtin with redirections, we need to fork */
    if (simple->redir_len > 0 && argv[0]) {
        int is_builtin_cmd = (strcmp(argv[0], "true") == 0 ||
                              strcmp(argv[0], "false") == 0 ||
                              strcmp(argv[0], "echo") == 0);

        if (is_builtin_cmd) {
            /* Fork even for builtin if redirections are present */
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                return 1;
            }

            if (pid == 0) {
                /* Child process: apply redirections then execute builtin */
                if (apply_redirections(simple->redirs, simple->redir_len) < 0) {
                    _exit(1);
                }
                if (try_builtin(argv, &st))
                    _exit(st);
                _exit(1);
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
    }

    /* Check if it's a builtin command (before fork) */
    if (try_builtin(argv, &st))
        return st;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child process: apply redirections then execute */
        if (apply_redirections(simple->redirs, simple->redir_len) < 0) {
            _exit(1);
        }
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
        return exec_simple(&n->as.simple);

    if (n->type == AST_LIST)
        return exec_list(n->as.list.items, n->as.list.len);

    if (n->type == AST_IF)
        return exec_if(&n->as.ifnode);

    return 1;
}
