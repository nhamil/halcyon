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

/**
 * Creates a vector wit the specified element type. 
 * 
 * @param v The vector 
 * @param type Element data type 
 */
#define CreateVectorType(v, type) CreateVector(v, sizeof(type))

/**
 * Pushes one element onto the end of the vector. 
 * 
 * @param v The vector
 * @param type Element data type
 * @param value Data to set the element to (not a pointer) 
 */
#define PushElemType(v, type, value) do { type vecTmpValue = (value); PushElem(v, &vecTmpValue); } while (0)

/**
 * Inserts an element at the specified index. 
 * 
 * @param v The vector 
 * @param type Element data type
 * @param index Array index 
 * @param value Data to set the element to (not a pointer) 
 */
#define InsertElemType(v, type, index, value) do { type vecTmpValue = (value); InsertElem(v, index, &vecTmpValue); } while (0)

/**
 * Gets a pointer to the `index`th element. 
 * 
 * @param v The vector 
 * @param type Element data type 
 * @param index Array index 
 * @return Pointer to element
 */
#define ElemAtType(v, type, index) (*(type*) ElemAt(v, index)) 

/**
 * Gets a const pointer to the `index`th element. 
 * 
 * @param v The vector 
 * @param type Element data type 
 * @param index Array index 
 * @return Const pointer to element
 */
#define ConstElemAtType(v, type, index) (*(const type*) ConstElemAt(v, index))

/**
 * Dynamic array for generic data types. 
 */
typedef struct Vector Vector; 

struct Vector 
{
    U64 Size; 
    U64 Capacity; 
    U64 ElemSize; 
    char* Data; 
};

/**
 * Initializes a vector with an element size of `elemSize`
 * 
 * @param v The vector 
 * @param elemSize Element size in bytes 
 */
static inline void CreateVector(Vector* v, U64 elemSize) 
{
    v->Size = 0; 
    v->Capacity = 16; 
    v->ElemSize = elemSize; 
    v->Data = malloc(v->Capacity * elemSize); 
}

/**
 * Initializes a vector to be a copy of another. 
 * 
 * @param v The vector 
 * @param src The vector to make a copy of 
 */
static inline void CreateVectorCopy(Vector* v, const Vector* src) 
{
    v->Size = src->Size; 
    v->Capacity = src->Capacity; 
    v->ElemSize = src->ElemSize; 
    v->Data = malloc(v->Capacity * v->ElemSize); 
    memcpy(v->Data, src->Data, v->Size * v->ElemSize); 
}

/**
 * Makes an (already initialized) vector a copy of another. 
 * 
 * @param v The vector 
 * @param src The vector to make a copy of 
 */
static inline void CopyVector(Vector* v, const Vector* src) 
{
    v->Size = src->Size; 
    v->Capacity = src->Capacity; 
    v->ElemSize = src->ElemSize; 
    v->Data = realloc(v->Data, v->Capacity * v->ElemSize); 
    memcpy(v->Data, src->Data, v->Size * v->ElemSize); 
}

/**
 * Frees vector memory. This does not perform any deinitialization of elements. 
 * 
 * @param v The vector 
 */
static inline void DestroyVector(Vector* v) 
{
    free(v->Data); 
}

/**
 * Reserves space. 
 * 
 * @param v The vector 
 * @param cap Element capacity 
 */
static inline void ReserveElems(Vector* v, U64 cap) 
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

/**
 * Gets a const pointer to the `index`th element. 
 * 
 * @param v The vector 
 * @param index Array index 
 * @return Const pointer to element
 */
static inline const void* ConstElemAt(const Vector* v, U64 index) 
{
    return &v->Data[v->ElemSize * index]; 
}

/**
 * Gets a pointer to the `index`th element. 
 * 
 * @param v The vector 
 * @param index Array index 
 * @return Pointer to element
 */
static inline void* ElemAt(Vector* v, U64 index) 
{
    return &v->Data[v->ElemSize * index]; 
}

/**
 * Pushes one element onto the end of the vector. 
 * 
 * @param v The vector
 * @param in Data to set the element to
 */
static inline void PushElem(Vector* v, const void* in) 
{
    ReserveElems(v, v->Size + 1); 
    memcpy(ElemAt(v, v->Size++), in, v->ElemSize); 
}

/**
 * Pushes an un initialized element to the end of the vector. 
 * 
 * @param v The vector 
 * @return Pointer to the new element
 */
static inline void* PushEmptyElem(Vector* v) 
{
    ReserveElems(v, v->Size + 1); 
    return ElemAt(v, v->Size++); 
}

/**
 * Pops one element from the end of the vector. 
 * 
 * @param v The vector 
 */
static inline void PopElem(Vector* v) 
{
    v->Size--; 
}

/**
 * Pops multiple elements from the end of the vector 
 * 
 * @param v The vector 
 * @param n Number of elements to pop 
 */
static inline void PopNElems(Vector* v, U64 n) 
{
    v->Size -= n; 
}

/**
 * Pops elements from the end of the vector such that the final size is the requested size. 
 * 
 * @param v The vector 
 * @param newSize Target size 
 */
static inline void PopElemsToSize(Vector* v, U64 newSize) 
{
    v->Size = newSize; 
}

/**
 * Copies element data. 
 * 
 * @param v The vector 
 * @param index Array index 
 * @param out Element data
 */
static inline void GetElem(const Vector* v, U64 index, void* out) 
{
    memcpy(out, ConstElemAt(v, index), v->ElemSize); 
}

/**
 * Sets the data of an existing element. 
 * 
 * @param v The vector 
 * @param index Array index 
 * @param in Element data
 */
static inline void SetElem(Vector* v, U64 index, const void* in) 
{
    memcpy(ElemAt(v, index), in, v->ElemSize); 
}

/**
 * Inserts an element at the specified index. 
 * 
 * @param v The vector 
 * @param index Array index 
 * @param in Element data
 */
static inline void InsertElem(Vector* v, U64 index, const void* in) 
{
    ReserveElems(v, v->Size + 1); 
    memmove(ElemAt(v, index + 1), ElemAt(v, index), v->ElemSize * (v->Size - index)); 
    v->Size++; 
    SetElem(v, index, in); 
}

/**
 * Removes all elements from the vector. 
 * 
 * @param v The vector 
 */
static inline void ClearVector(Vector* v) 
{
    v->Size = 0; 
}

/**
 * Swaps the indices of two elements. 
 * 
 * @param v The vector 
 * @param a First array index 
 * @param b Second array index 
 */
static inline void SwapElems(Vector* v, U64 a, U64 b) 
{
    char tmp[v->ElemSize]; 
    GetElem(v, a, tmp); 
    GetElem(v, b, ElemAt(v, a)); 
    SetElem(v, b, tmp); 
}

/**
 * Checks if the vector contains the specified element. 
 * 
 * @param v The vector 
 * @param in Element to search for 
 * @param idx First instance of the element index. Only set if it is found. 
 * @return True if the element is found, otherwise false. 
 */
static inline bool ContainsElem(const Vector* v, const void* in, U64* idx) 
{
    for (U64 i = 0; i < v->Size; i++) 
    {
        if (memcmp(in, ConstElemAt(v, i), v->ElemSize) == 0) 
        {
            if (idx) *idx = i; 
            return true; 
        }
    }

    return false; 
}
