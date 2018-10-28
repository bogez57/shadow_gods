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

#define Roundup32(x) (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4, (x) |= (x) >> 8, (x) |= (x) >> 16, ++(x))
#define ResizeArr(type, array, size) ((array).capacity = (size), (array).elements = (type*)ReAlloc((array).elements, type, (array).capacity))
#define CopyArr(type, newArr, originalArr)                                                    \
    do                                                                                        \
    {                                                                                         \
        if ((newArr).capacity < (originalArr).capacity)                                       \
            ResizeArr(type, newArr, (originalArr).size);                                      \
        (newArr).size = (originalArr).size;                                                   \
        memcpy((newArr).elements, (originalArr).elements, sizeof(type) * (originalArr).size); \
    } while (0)

//Resize array if appending over bounds
#define DynamicAppend(type, array, element)                                              \
    do                                                                                   \
    {                                                                                    \
        if ((array).size == (array).capacity)                                            \
        {                                                                                \
            (array).capacity = (array).capacity ? (array).capacity << 1 : 2;             \
            (array).elements = (type*)ReAlloc((array).elements, type, (array).capacity); \
        }                                                                                \
        (array).elements[(array).size++] = (element);                                    \
    } while (0)

#define kv_pushp(type, array) (((array).size == (array).capacity) ? ((array).capacity = ((array).capacity ? (array).capacity << 1 : 2),                   \
                                                                        (array).elements = (type*)ReAlloc((array).elements, (type), (array).capacity), 0) \
                                                                  : 0),                                                                                   \
                              ((array).elements + ((array).size++))

//Resize array if inserting over bounds
#define DynamicInsert(type, array, i) (((array).capacity <= (size_t)(i) ? ((array).capacity = (array).size = (i) + 1, Roundup32((array).capacity),            \
                                                                              (array).elements = (type*)ReAlloc((array).elements, type, (array).capacity), 0) \
                                                                        : (array).size <= (size_t)(i) ? (array).size = (i) + 1                                \
                                                                                                      : 0),                                                   \
    (array).elements[(i)])

template <typename Type>
class Dynam_Array
{
public:
    Dynam_Array() = default;
    Dynam_Array(size_t initialSize)
        : capacity(initialSize)
    {
        ResizeArr(Type, *this, capacity);
        memset(this->elements, 0, initialSize);
    };

    void Insert(Type element, ui32 AtIndex)
    {
        DynamicInsert(Type, *this, AtIndex) = element;
    };
    void PushBack(Type element)
    {
        DynamicAppend(Type, *this, element);
    };
    void PopBack()
    {
        this->elements[size - 1].x = 0;
        this->elements[size - 1].y = 0;
        this->elements[--this->size];
    };
    Type At(ui32 Index)
    {
        BGZ_ASSERT(Index < this->size, "Trying to access index out of current array bounds! Is it because array has been manually destroyed: %s", hasArrayBeenDestroyed ? "Yes" : "No");
        Type result = this->elements[Index];
        return result;
    };
    void Destroy()
    {
        DeAlloc(this->elements);
        this->size = 0;
        this->capacity = 0;
        hasArrayBeenDestroyed = true;
    };

    size_t size {}, capacity {};
    b      hasArrayBeenDestroyed { false };
    Type*  elements { nullptr };
};

template <typename Type>
Dynam_Array<Type> CopyArray(Dynam_Array<Type> sourceArray, Dynam_Array<Type> destinationArray)
{
    CopyArr(Type, destinationArray, sourceArray);

    return destinationArray;
};

template <typename Type>
void SwapArrays(Dynam_Array<Type>* array1, Dynam_Array<Type>* array2)
{
    Dynam_Array<Type>* arrayTemp = array1;
    array1 = array2;
    array2 = arrayTemp;
};
