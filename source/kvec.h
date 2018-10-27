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
  An example:

#include "kvec.h"
int main() {
	kvec_t(int) array;
	kv_init(array);
	kv_push(int, array, 10); // append
	kv_a(int, array, 20) = 5; // dynamic
	kv_A(array, 20) = 4; // static
	kv_destroy(array);
	return 0;
}
*/

/*
  2008-09-22 (0.1.0):

	* The initial version.

*/

#pragma once

#include <stdlib.h>

#define kv_roundup32(x) (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4, (x) |= (x) >> 8, (x) |= (x) >> 16, ++(x))

#define kv_destroy(array) free((array).elements)
#define kv_A(array, i) ((array).elements[(i)])
#define kv_pop(array) ((array).elements[--(array).size])
#define kv_size(array) ((array).size)
#define kv_max(array) ((array).capacity)

#define kv_resize(type, array, size) ((array).capacity = (size), (array).elements = (type*)ReAlloc((array).elements, type, (array).capacity))

#define kv_copy(type, newArr, originalArr)                                                    \
    do                                                                                        \
    {                                                                                         \
        if ((newArr).capacity < (originalArr).capacity)                                       \
            kv_resize(type, newArr, (originalArr).size);                                      \
        (newArr).n = (originalArr).size;                                                      \
        memcpy((newArr).elements, (originalArr).elements, sizeof(type) * (originalArr).size); \
    } while (0)

#define kv_push(type, array, element)                                                    \
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

#define kv_a(type, array, i) (((array).capacity <= (size_t)(i) ? ((array).capacity = (array).size = (i) + 1, kv_roundup32((array).capacity),         \
                                                                     (array).elements = (type*)ReAlloc((array).elements, type, (array).capacity), 0) \
                                                               : (array).size <= (size_t)(i) ? (array).size = (i) + 1                                \
                                                                                             : 0),                                                   \
    (array).elements[(i)])

template <typename T>
class Dynam_Array
{
public:
    Dynam_Array() = default;
    Dynam_Array(size_t initialSize)
        : capacity(initialSize)
    {
        kv_resize(T, *this, capacity);
        memset(this->elements, 0, initialSize);
    };

    void InitArray() {};
    void Push(T element)
    {
        kv_push(T, *this, element);
    };
    T At(ui32 Index)
    {
        return kv_a(T, *this, Index);
    };

    size_t size {}, capacity {};
    T*     elements { nullptr };
};
