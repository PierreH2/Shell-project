#include "vec.h"
#include <stdlib.h>

void vec_init(struct vec *v)
{
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

void vec_free(struct vec *v)
{
    free(v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

void *vec_get(struct vec *v, size_t i)
{
    return v->data[i];
}

void vec_push(struct vec *v, void *ptr)
{
    if (v->len == v->cap) {
        size_t new_cap = (v->cap == 0) ? 8 : (v->cap * 2);
        void **nd = realloc(v->data, new_cap * sizeof(void *));
        if (!nd)
            abort();
        v->data = nd;
        v->cap = new_cap;
    }
    v->data[v->len++] = ptr;
}

