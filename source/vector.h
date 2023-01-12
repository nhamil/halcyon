#pragma once 

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#define VECTOR_CREATE_TYPE(v, type) vector_create(v, sizeof(type))
#define VECTOR_PUSH_TYPE(v, type, value) do { type vector_tmp_value = (value); vector_push(v, &vector_tmp_value); } while (0)
#define VECTOR_AT_TYPE(v, type, index) *(type *) vector_at(v, index) 

typedef struct vector vector; 

struct vector 
{
    size_t size; 
    size_t capacity; 
    size_t elem_size; 
    char *data; 
};

static inline void vector_create(vector *v, size_t elem_size) 
{
    v->size = 0; 
    v->capacity = 16; 
    v->elem_size = elem_size; 
    v->data = malloc(v->capacity * elem_size); 
}

static inline void vector_destroy(vector *v) 
{
    free(v->data); 
}

static inline void vector_reserve(vector *v, size_t cap) 
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

static inline const void *vector_at_const(const vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void *vector_at(vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void vector_push(vector *v, const void *in) 
{
    vector_reserve(v, v->size); 
    memcpy(vector_at(v, v->size), in, v->elem_size); 
    v->size++; 
}

static inline void *vector_push_empty(vector *v) 
{
    vector_reserve(v, v->size + 1); 
    return vector_at(v, v->size++); 
}

static inline void vector_pop(vector *v) 
{
    v->size--; 
}

static inline void vector_popn(vector *v, size_t n) 
{
    v->size -= n; 
}

static inline void vector_pop_to_size(vector *v, size_t new_size) 
{
    v->size = new_size; 
}

static inline void vector_get(const vector *v, size_t index, void *out) 
{
    memcpy(out, vector_at_const(v, index), v->elem_size); 
}

static inline void vector_set(vector *v, size_t index, const void *in) 
{
    memcpy(vector_at(v, index), in, v->elem_size); 
}

static inline void vector_clear(vector *v) 
{
    v->size = 0; 
}
