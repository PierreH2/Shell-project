#include "str.h"
#include <stdlib.h>
#include <string.h>

void str_init(struct str *s)
{
    s->buf = NULL;
    s->len = 0;
    s->cap = 0;
}

void str_free(struct str *s)
{
    free(s->buf);
    s->buf = NULL;
    s->len = 0;
    s->cap = 0;
}

static void ensure_cap(struct str *s, size_t add)
{
    size_t need = s->len + add + 1;
    if (need <= s->cap)
        return;
    size_t nc = (s->cap == 0) ? 32 : s->cap;
    while (nc < need)
        nc *= 2;
    char *nb = realloc(s->buf, nc);
    if (!nb)
        abort();
    s->buf = nb;
    s->cap = nc;
}

void str_pushc(struct str *s, char c)
{
    ensure_cap(s, 1);
    s->buf[s->len++] = c;
    s->buf[s->len] = '\0';
}

void str_append(struct str *s, const char *t)
{
    size_t n = strlen(t);
    ensure_cap(s, n);
    memcpy(s->buf + s->len, t, n);
    s->len += n;
    s->buf[s->len] = '\0';
}

char *str_take(struct str *s)
{
    if (!s->buf) {
        char *z = malloc(1);
        if (!z)
            abort();
        z[0] = '\0';
        return z;
    }
    char *out = s->buf;
    s->buf = NULL;
    s->len = 0;
    s->cap = 0;
    return out;
}

