#include <boagz/error_handling.h>

template <typename Type, i64 initialSize = 1>
class Array
{
public:
    Type& operator[](i64 index)
    {
        return this->elements[index];
    };

    inline Type& At(i64 index)
    {
        BGZ_ASSERT(index < initialSize, "Attempting to access index %i which is out of current array bounds - current max array index: %i", index, initialSize - 1);
        return (Type&)this->elements[index];
    };

    inline Type& Front()
    {
        BGZ_ASSERT(NOT Empty(), "Attempting to access empty array!");
        return this->elements[0];
    };

    inline b Empty() const
    {
        return (this->initialSize == 0);
    };

    Type elements[initialSize ? initialSize : 1];
};