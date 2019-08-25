/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
  2008-09-22 (0.1.0):

	* The initial version.

*/

/*
    TODO:
     1.) Currently, dynamic array always intializes elements to zero through constructor and Init func. If you want to 
     give the option of more speed you will need to add a method similar to std::vector reserve(). The idea is you give 
     the option to allocate memory (just call malloc) but don't initialize it to 0. 
*/

#pragma once

#include "dynamic_allocator.h"

#define Roundup32(x) (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4, (x) |= (x) >> 8, (x) |= (x) >> 16, ++(x))

//Not quite sure what this does yet
/*
#define kv_pushp(type, array) (((array).size == (array).capacity) ? ((array).capacity = ((array).capacity ? (array).capacity << 1 : 2),                   \
                                                                        (array).elements = (type*)ReAlloc((array).elements, (type), (array).capacity), 0) \
                                                                  : 0),                                                                                   \
                              ((array).elements + ((array).size++))
*/

template <typename Type>
class Dynam_Array
{
public:
    Dynam_Array() = default;
    Dynam_Array(i32 memPartitionID_dynamic) 
        : memPartitionID(memPartitionID_dynamic)
    {}
    Dynam_Array(i64 initialSize, const Type& type, i32 memPartitionID_dynamic)
        : capacity(initialSize)
        , memPartitionID(memPartitionID_dynamic)
    {
        ResizeArray<Type>($(*this), initialSize);
        memset(this->elements, 0, initialSize); 
        this->size = initialSize;

        for(i32 i{}; i < this->size; ++i)
        {
            this->elements[i] = type;
        };
    };
    Dynam_Array(i64 initialSize, i32 memPartitionID_dynamic)
        : capacity(initialSize)
        , memPartitionID(memPartitionID_dynamic)
    {
        ResizeArray<Type>($(*this), initialSize);
        memset(this->elements, 0, initialSize); 
        this->size = initialSize;
    };

    Type& operator[](i64 index)
    {
        BGZ_ASSERT(index < this->capacity, "Attempting to access index %i which is out of current dynam array bounds - current max array capacity: %i", index, capacity);
        BGZ_ASSERT(index < this->size, "Attempting to access index %i which is out of current dynam array bounds - current max array size: %i", index, size);
        return (this->elements[index]);
    };

    Type& At(i64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which exceeds capacity - current max array capacity: %i", index, capacity);
        BGZ_ASSERT(index < this->size, "Attempting to access index %i which exceeds current array size - current max array size: %i", index, size);
        return (this->elements[index]);
    };

    i64 size {}, capacity {};
    b hasArrayBeenDestroyed { false };
    Type* elements { nullptr };
    i32 memPartitionID{};
};

template <typename Type>
void PopBack(Dynam_Array<Type>&& arr)
{
    BGZ_ASSERT(arr.size > 0, "Cannot pop off an empty dynamic array!");
    arr.elements[arr.size - 1] = {};
    arr.elements[--arr.size];
};

//Resize array if appending over bounds
template <typename Type>
void PushBack(Dynam_Array<Type>&& arr, Type element)
{
    if (arr.size == arr.capacity)
    {
        arr.capacity = arr.capacity ? arr.capacity << 1 : 2;
        arr.elements = (Type*)ReAllocSize(arr.memPartitionID, arr.elements, sizeof(Type) * arr.capacity);
    }

    arr.elements[arr.size++] = (element);
};

template <typename Type>
void Insert(Dynam_Array<Type>&& arr, Type element, ui32 AtIndex)
{
    ((arr.capacity <= (i64)(AtIndex) ? (arr.capacity = arr.size = (AtIndex) + 1,
                                            Roundup32(arr.capacity),
                                            arr.elements = (Type*)ReAllocSize(arr.memPartitionID, arr.elements, sizeof(Type) * arr.capacity),
                                            0)
                                            : arr.size <= (i64)(AtIndex) ? arr.size = (AtIndex) + 1 : 0),
            arr.elements[(AtIndex)])
            = element;
};

template <typename Type>
void Reserve(Dynam_Array<Type>&& arr, ui32 numItems)
{
    i64 newSize = arr.capacity + numItems;
    ResizeArray<Type>($(arr), newSize);
};

template <typename Type>
void Destroy(Dynam_Array<Type>&& arr)
{
    DeAlloc(memPartitionID, arr.elements);
    arr.size = 0;
    arr.capacity = 0;
    hasArrayBeenDestroyed = true;
};

template <typename Type>
b IsEmpty(Dynam_Array<Type>&& arr)
{
    return (arr.size == 0) ? true : false;
};

template <typename Type>
void ResizeArray(Dynam_Array<Type>&& arrayToResize, i64 size)
{
    (arrayToResize.capacity = (size), arrayToResize.elements = (Type*)ReAllocSize(arrayToResize.memPartitionID, arrayToResize.elements, (sizeof(Type) * arrayToResize.capacity)));
};

template <typename Type>
Dynam_Array<Type> CopyArray(Dynam_Array<Type> sourceArray, Dynam_Array<Type> destinationArray)
{
    if (destinationArray.capacity < sourceArray.capacity)
    {
        ResizeArray<Type>(destinationArray, sourceArray.size);
    };

    destinationArray.size = sourceArray.size;
    memcpy(destinationArray.elements, sourceArray.elements, sizeof(Type) * sourceArray.size);

    return destinationArray;
};

template <typename Type>
void SwapArrays(Dynam_Array<Type>* array1, Dynam_Array<Type>* array2)
{
    Dynam_Array<Type>* arrayTemp = array1;
    array1 = array2;
    array2 = arrayTemp;
};
