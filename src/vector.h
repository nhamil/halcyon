#pragma once 

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#define CREATE_VEC(v, type) create_vec(v, sizeof(type))
#define PUSH_VEC(v, type, value) do { type vec_tmp_value = (value); push_vec(v, &vec_tmp_value); } while (0)
#define INSERT_VEC(v, type, index, value) do { type vec_tmp_value = (value); insert_vec(v, index, &vec_tmp_value); } while (0)
#define AT_VEC(v, type, index) *(type *) at_vec(v, index) 

typedef struct vector vector; 

struct vector 
{
    size_t size; 
    size_t capacity; 
    size_t elem_size; 
    char *data; 
};

static inline void create_vec(vector *v, size_t elem_size) 
{
    v->size = 0; 
    v->capacity = 16; 
    v->elem_size = elem_size; 
    v->data = malloc(v->capacity * elem_size); 
}

static inline void destroy_vec(vector *v) 
{
    free(v->data); 
}

static inline void reserve_vec(vector *v, size_t cap) 
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

static inline const void *at_vec_const(const vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void *at_vec(vector *v, size_t index) 
{
    return &v->data[v->elem_size * index]; 
}

static inline void push_vec(vector *v, const void *in) 
{
    reserve_vec(v, v->size + 1); 
    memcpy(at_vec(v, v->size++), in, v->elem_size); 
}

static inline void *push_vec_empty(vector *v) 
{
    reserve_vec(v, v->size + 1); 
    return at_vec(v, v->size++); 
}

static inline void pop_vec(vector *v) 
{
    v->size--; 
}

static inline void pop_vecn(vector *v, size_t n) 
{
    v->size -= n; 
}

static inline void pop_vec_to_size(vector *v, size_t new_size) 
{
    v->size = new_size; 
}

static inline void get_vec(const vector *v, size_t index, void *out) 
{
    memcpy(out, at_vec_const(v, index), v->elem_size); 
}

static inline void set_vec(vector *v, size_t index, const void *in) 
{
    memcpy(at_vec(v, index), in, v->elem_size); 
}

static inline void insert_vec(vector *v, size_t index, const void *in) 
{
    reserve_vec(v, v->size + 1); 
    memmove(at_vec(v, index + 1), at_vec(v, index), v->elem_size * (v->size - index)); 
    v->size++; 
    set_vec(v, index, in); 
}

static inline void clear_vec(vector *v) 
{
    v->size = 0; 
}

static inline void swap_vec(vector *v, size_t a, size_t b) 
{
    char tmp[v->elem_size]; 
    get_vec(v, a, tmp); 
    get_vec(v, b, at_vec(v, a)); 
    set_vec(v, b, tmp); 
}
