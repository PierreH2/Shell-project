#ifndef STR_H
#define STR_H

#include <stddef.h>

struct str {
    char *buf;
    size_t len;
    size_t cap;
};

void str_init(struct str *s);
void str_pushc(struct str *s, char c);
void str_append(struct str *s, const char *t);
char *str_take(struct str *s); // retourne malloced string et reset
void str_free(struct str *s);

#endif
