#ifndef VEC_H
#define VEC_H

#include <stddef.h>

struct vec {
    void **data;
    size_t len;
    size_t cap;
};

void vec_init(struct vec *v);
void vec_push(struct vec *v, void *ptr);
void *vec_get(struct vec *v, size_t i);
void vec_free(struct vec *v);

#endif
