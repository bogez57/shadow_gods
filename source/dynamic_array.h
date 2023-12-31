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
    Dynam_Array(s32 memPartitionID_dynamic)
        : memPartitionID(memPartitionID_dynamic)
    {}
    Dynam_Array(s64 initialSize, s32 memPartitionID_dynamic)
        : capacity(initialSize)
        , memPartitionID(memPartitionID_dynamic)
    {
        ResizeArray<Type>($(*this), initialSize);
        memset(this->elements, 0, initialSize);
        this->size = initialSize;
    };
    Dynam_Array(s64 initialSize, const Type& type, s32 memPartitionID_dynamic)//Have default initialized elements
        : capacity(initialSize)
        , memPartitionID(memPartitionID_dynamic)
    {
        ResizeArray<Type>($(*this), initialSize);
        memset(this->elements, 0, initialSize);
        this->size = initialSize;
        
        for(s32 i{}; i < this->size; ++i)
        {
            this->elements[i] = type;
        };
    };
    
    Type& operator[](s64 index)
    {
        BGZ_ASSERT(index >= 0, "Cannot access a negative index!");
        BGZ_ASSERT(index < this->capacity, "Attempting to access an index which is outside of current dynam array bounds!");
        BGZ_ASSERT(index < this->size, "Attempting to access an index which is outside of current dynam array bounds!");
        
        return (this->elements[index]);
    };
    
    Type& At(s64 index)
    {
        BGZ_ASSERT(index >= 0, "Cannot access a negative index!");
        BGZ_ASSERT(index < capacity, "Attempting to access an index which exceeds capacity!");
        BGZ_ASSERT(index < this->size, "Attempting to access an index which exceeds current array size!");
        
        return (this->elements[index]);
    };
    
    s64 size {}, capacity {};
    bool hasArrayBeenDestroyed { false };
    Type* elements { nullptr };
    s32 memPartitionID{0};
};

template <typename Type>
void Initialize(Dynam_Array<Type>&& arr, s32 memPartitionID_dynamic)
{
    arr.memPartitionID = memPartitionID_dynamic;
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
void Insert(Dynam_Array<Type>&& arr, Type element, s32 AtIndex)
{
    BGZ_ASSERT(AtIndex >= 0, "Cannot access a negative index!");
    
    ((arr.capacity <= (s64)(AtIndex) ? (arr.capacity = arr.size = (AtIndex) + 1,
                                        Roundup32(arr.capacity),
                                        arr.elements = (Type*)ReAllocSize(arr.memPartitionID, arr.elements, sizeof(Type) * arr.capacity),
                                        0)
      : arr.size <= (s64)(AtIndex) ? arr.size = (AtIndex) + 1 : 0),
     arr.elements[(AtIndex)])
        = element;
};

template <typename Type>
void Reserve(Dynam_Array<Type>&& arr, s32 numItems)
{
    BGZ_ASSERT(numItems >= 0, "Cannot reserve a negative number of items!");
    
    s64 newSize = arr.capacity + numItems;
    ResizeArray<Type>($(arr), newSize);
};

template <typename Type>
void CleanUp(Dynam_Array<Type>&& arr)
{
    DeAlloc(arr.memPartitionID, arr.elements);
    arr.size = 0;
    arr.capacity = 0;
    arr.hasArrayBeenDestroyed = true;
    arr.memPartitionID = 0;
};

template <typename Type>
b IsEmpty(Dynam_Array<Type>&& arr)
{
    return (arr.size == 0) ? true : false;
};

template <typename Type>
Type* GetLastElem(Dynam_Array<Type> arr)
{
    BGZ_ASSERT(arr.size != 0, "Nothing has been pushed or insereted onto array");
    
    Type* lastElem = &arr.elements[arr.size - 1];
    
    return lastElem;
};

template <typename Type>
void ResizeArray(Dynam_Array<Type>&& arrayToResize, s64 size)
{
    (arrayToResize.capacity = (size), arrayToResize.elements = (Type*)ReAllocSize(arrayToResize.memPartitionID, arrayToResize.elements, (sizeof(Type) * arrayToResize.capacity)));
};

template <typename Type>
void CopyArray(Dynam_Array<Type> sourceArray, Dynam_Array<Type>&& destinationArray)
{
    if (destinationArray.capacity < sourceArray.capacity)
    {
        ResizeArray<Type>($(destinationArray), sourceArray.size);
    };
    
    BGZ_ASSERT(destinationArray.size == sourceArray.size, "Did not resize arrays correctly when copying!");
    
    if(destinationArray.elements == sourceArray.elements)
    {
        destinationArray.elements = MallocType(heap, Type, destinationArray.size);
    };
    
    memcpy(destinationArray.elements, sourceArray.elements, sizeof(Type) * sourceArray.size);
};

template <typename Type>
void SwapArrays(Dynam_Array<Type>* array1, Dynam_Array<Type>* array2)
{
    Dynam_Array<Type>* arrayTemp = array1;
    array1 = array2;
    array2 = arrayTemp;
};
