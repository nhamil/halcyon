#pragma once 

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#define VEC_CREATE(v, type) vec_create(v, sizeof(type))
#define VEC_PUSH(v, type, value) do { type vec_tmp_value = (value); vec_push(v, &vec_tmp_value); } while (0)
#define VEC_INSERT(v, type, index, value) do { type vec_tmp_value = (value); vec_insert(v, index, &vec_tmp_value); } while (0)
#define VEC_AT(v, type, index) *(type *) vec_at(v, index) 

typedef struct vector vector; 

struct vector 
{
    size_t size; 
    size_t capacity; 
    size_t elem_size; 
    char *data; 
};

static inline void vec_create(vector *v, size_t elem_size) 
{
    v->size = 0; 
    v->capacity = 16; 
    v->elem_size = elem_size; 
    v->data = malloc(v->capacity * elem_size); 
}

static inline void vec_destroy(vector *v) 
{
    free(v->data); 
}

static inline void vec_reserve(vector *v, size_t cap) 
{
    size_t new_cap = v->capacity; 
    while (new_cap < cap) 
    {
        new_cap *= 2; 
    }
    if (new_cap != v->capacity) 
    {
        v->capacity = new_cap; 
        v->data = realloc(v->data, v->elem_size * new_cap); 
    }
}

static inline const void *vec_at_const(const vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void *vec_at(vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void vec_push(vector *v, const void *in) 
{
    vec_reserve(v, v->size + 1); 
    memcpy(vec_at(v, v->size++), in, v->elem_size); 
}

static inline void *vec_pushe(vector *v) 
{
    vec_reserve(v, v->size + 1); 
    return vec_at(v, v->size++); 
}

static inline void vec_pop(vector *v) 
{
    v->size--; 
}

static inline void vec_popn(vector *v, size_t n) 
{
    v->size -= n; 
}

static inline void vec_pop_size(vector *v, size_t new_size) 
{
    v->size = new_size; 
}

static inline void vec_get(const vector *v, size_t index, void *out) 
{
    memcpy(out, vec_at_const(v, index), v->elem_size); 
}

static inline void vec_set(vector *v, size_t index, const void *in) 
{
    memcpy(vec_at(v, index), in, v->elem_size); 
}

static inline void vec_insert(vector *v, size_t index, const void *in) 
{
    vec_reserve(v, v->size + 1); 
    memmove(vec_at(v, index + 1), vec_at(v, index), v->elem_size * (v->size - index)); 
    v->size++; 
    vec_set(v, index, in); 
}

static inline void vec_clear(vector *v) 
{
    v->size = 0; 
}
