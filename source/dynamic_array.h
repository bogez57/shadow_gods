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
        *this = ResizeArray<Type>(*this, initialSize);
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
        *this = ResizeArray<Type>(*this, initialSize);
        memset(this->elements, 0, initialSize); 
        this->size = initialSize;
    };

    Type& operator[](i64 index)
    {
        BGZ_ASSERT(index < this->capacity, "Attempting to access index %i which is out of current dynam array bounds - current max array capacity: %i", index, capacity);
        BGZ_ASSERT(index < this->size, "Attempting to access index %i which is out of current dynam array bounds - current max array size: %i", index, size);
        return *(this->elements + index);
    };

    Type& At(i64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which exceeds capacity - current max array capacity: %i", index, capacity);
        BGZ_ASSERT(index < this->size, "Attempting to access index %i which exceeds current array size - current max array size: %i", index, size);
        return *(this->elements + index);
    };

    void Reserve(ui32 numItems)
    {
        i64 newSize = this->capacity + numItems;
        *this = ResizeArray<Type>(*this, newSize);
    };

    void Insert(Type element, ui32 AtIndex)
    {
        ((this->capacity <= (i64)(AtIndex) ? (this->capacity = this->size = (AtIndex) + 1,
                                                 Roundup32(this->capacity),
                                                 this->elements = (Type*)ReAllocSize(this->memPartitionID, this->elements, sizeof(Type) * this->capacity),
                                                 0)
                                           : this->size <= (i64)(AtIndex) ? this->size = (AtIndex) + 1 : 0),
            this->elements[(AtIndex)])
            = element;
    };

    //Resize array if appending over bounds
    void PushBack(Type element)
    {
        if (this->size == this->capacity)
        {
            this->capacity = this->capacity ? this->capacity << 1 : 2;
            this->elements = (Type*)ReAllocSize(this->memPartitionID, this->elements, sizeof(Type) * this->capacity);
        }
        this->elements[this->size++] = (element);
    };

    void PopBack()
    {
        BGZ_ASSERT(this->size > 0, "Cannot pop off an empty dynamic array!");
        this->elements[this->size - 1] = {};
        this->elements[--this->size];
    };

    void Destroy()
    {
        DeAlloc(memPartitionID, this->elements);
        this->size = 0;
        this->capacity = 0;
        hasArrayBeenDestroyed = true;
    };

    b Empty()
    {
        return (size == 0) ? true : false;
    };

    i64 size {}, capacity {};
    b hasArrayBeenDestroyed { false };
    Type* elements { nullptr };
    i32 memPartitionID;
};

template <typename Type>
Dynam_Array<Type> ResizeArray(Dynam_Array<Type> arrayToResize, i64 size)
{
    (arrayToResize.capacity = (size), arrayToResize.elements = (Type*)ReAllocSize(arrayToResize.memPartitionID, arrayToResize.elements, (sizeof(Type) * arrayToResize.capacity)));

    return arrayToResize;
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
