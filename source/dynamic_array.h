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

//TODO: Need to overload brackets [] operator for class as long as array's elements are accesible

#pragma once

#include <stdlib.h>
#include "shared.h"
#include "memory_handling.h"

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

    Dynam_Array(i64 initialSize)
        : capacity(initialSize)
    {
        *this = ResizeArray<Type>(*this, initialSize);
        memset(this->elements, 0, initialSize);
    };

    inline Type& operator[](i64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which is out of current dynam array bounds - current max array index: %i", index, capacity - 1);
        return *(this->elements + Index);
    };

    inline Type& At(ui32 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which is out of current dynam array bounds - current max array index: %i", index, capacity - 1);
        return *(this->elements + index);
    };

    void Init(i64 initialSize)
    {
        *this = ResizeArray<Type>(*this, initialSize);
        memset(this->elements, 0, initialSize);
    };

    void Insert(Type element, ui32 AtIndex)
    {
        ((this->capacity <= (i64)(AtIndex) ? (this->capacity = this->size = (AtIndex) + 1,
                                                 Roundup32(this->capacity),
                                                 this->elements = (Type*)ReAlloc(this->elements, Type, this->capacity),
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
            this->elements = (Type*)ReAlloc(this->elements, Type, this->capacity);
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
        DeAlloc(this->elements);
        this->size = 0;
        this->capacity = 0;
        hasArrayBeenDestroyed = true;
    };

    i64 size {}, capacity {};
    b hasArrayBeenDestroyed { false };
    Type* elements { nullptr };
};

template <typename Type>
Dynam_Array<Type> ResizeArray(Dynam_Array<Type> arrayToResize, i64 size)
{
    (arrayToResize.capacity = (size), arrayToResize.elements = (Type*)ReAlloc(arrayToResize.elements, Type, arrayToResize.capacity));

    return arrayToResize;
};

template <typename Type>
Dynam_Array<Type> CopyArray(Dynam_Array<Type> sourceArray, Dynam_Array<Type> destinationArray)
{
    if (destinationArray.capacity < sourceArray.capacity)
    {
        ResizeArr(Type, destinationArray, sourceArray.size);
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
