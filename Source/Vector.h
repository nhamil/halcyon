/**
 * @file Vector.h
 * @author Nicholas Hamilton 
 * @date 2023-01-12
 * 
 * Copyright (c) 2023 Nicholas Hamilton
 * 
 * Dynamically allocated array for general storage. 
 */

#pragma once 

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#define CREATE_VEC(v, type) CreateVec(v, sizeof(type))
#define PUSH_VEC(v, type, value) do { type vecTmpValue = (value); PushVec(v, &vecTmpValue); } while (0)
#define INSERT_VEC(v, type, index, value) do { type vecTmpValue = (value); InsertVec(v, index, &vecTmpValue); } while (0)
#define AT_VEC(v, type, index) (*(type*) AtVec(v, index)) 
#define AT_VEC_CONST(v, type, index) (*(const type*) AtVecConst(v, index))

typedef struct Vector Vector; 

struct Vector 
{
    U64 Size; 
    U64 Capacity; 
    U64 ElemSize; 
    char* Data; 
};

static inline void CreateVec(Vector* v, U64 elemSize) 
{
    v->Size = 0; 
    v->Capacity = 16; 
    v->ElemSize = elemSize; 
    v->Data = malloc(v->Capacity * elemSize); 
}

static inline void CreateVecCopy(Vector* v, const Vector* src) 
{
    v->Size = src->Size; 
    v->Capacity = src->Capacity; 
    v->ElemSize = src->ElemSize; 
    v->Data = malloc(v->Capacity * v->ElemSize); 
    memcpy(v->Data, src->Data, v->Size * v->ElemSize); 
}

static inline void CopyVec(Vector* v, const Vector* src) 
{
    v->Size = src->Size; 
    v->Capacity = src->Capacity; 
    v->ElemSize = src->ElemSize; 
    v->Data = realloc(v->Data, v->Capacity * v->ElemSize); 
    memcpy(v->Data, src->Data, v->Size * v->ElemSize); 
}

static inline void DestroyVec(Vector* v) 
{
    free(v->Data); 
}

static inline void ReserveVec(Vector* v, U64 cap) 
{
    U64 newCap = v->Capacity; 
    while (newCap < cap) 
    {
        newCap *= 2; 
    }
    if (newCap != v->Capacity) 
    {
        v->Capacity = newCap; 
        v->Data = realloc(v->Data, v->ElemSize * newCap); 
    }
}

static inline const void* AtVecConst(const Vector* v, U64 index) 
{
    return &v->Data[v->ElemSize * index]; 
}

static inline void* AtVec(Vector* v, U64 index) 
{
    return &v->Data[v->ElemSize * index]; 
}

static inline void PushVec(Vector* v, const void* in) 
{
    ReserveVec(v, v->Size + 1); 
    memcpy(AtVec(v, v->Size++), in, v->ElemSize); 
}

static inline void* PushVecEmpty(Vector* v) 
{
    ReserveVec(v, v->Size + 1); 
    return AtVec(v, v->Size++); 
}

static inline void PopVec(Vector* v) 
{
    v->Size--; 
}

static inline void PopVecN(Vector* v, U64 n) 
{
    v->Size -= n; 
}

static inline void PopVecToSize(Vector* v, U64 newSize) 
{
    v->Size = newSize; 
}

static inline void GetVec(const Vector* v, U64 index, void* out) 
{
    memcpy(out, AtVecConst(v, index), v->ElemSize); 
}

static inline void SetVec(Vector* v, U64 index, const void* in) 
{
    memcpy(AtVec(v, index), in, v->ElemSize); 
}

static inline void InsertVec(Vector* v, U64 index, const void* in) 
{
    ReserveVec(v, v->Size + 1); 
    memmove(AtVec(v, index + 1), AtVec(v, index), v->ElemSize * (v->Size - index)); 
    v->Size++; 
    SetVec(v, index, in); 
}

static inline void ClearVec(Vector* v) 
{
    v->Size = 0; 
}

static inline void SwapVec(Vector* v, U64 a, U64 b) 
{
    char tmp[v->ElemSize]; 
    GetVec(v, a, tmp); 
    GetVec(v, b, AtVec(v, a)); 
    SetVec(v, b, tmp); 
}

static inline bool ContainsVec(const Vector* v, const void* in, U64* idx) 
{
    for (U64 i = 0; i < v->Size; i++) 
    {
        if (memcmp(in, AtVecConst(v, i), v->ElemSize) == 0) 
        {
            if (idx) *idx = i; 
            return true; 
        }
    }

    return false; 
}
