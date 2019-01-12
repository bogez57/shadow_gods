#include <boagz/error_handling.h>

template <typename Type, i64 size = 1>
class Array
{
public:
    Type& operator[](i64 index)
    {
        BGZ_ASSERT(index < size, "Attempting to access index %i which is out of current array bounds - current max array index: %i", index, size - 1);
        return this->elements[index];
    };

    inline Type& At(i64 index)
    {
        BGZ_ASSERT(index < size, "Attempting to access index %i which is out of current array bounds - current max array index: %i", index, size - 1);
        return (Type&)this->elements[index];
    };

    inline Type& Front()
    {
        BGZ_ASSERT(NOT Empty(), "Attempting to access empty array!");
        return this->elements[0];
    };

    inline b Empty() const
    {
        return (size == 0);
    };

    inline i64 Size() const
    {
        return size;
    }

    Type elements[size ? size : 1];
};