#include <boagz/error_handling.h>

template <typename Type, s64 size = 1>
class Array
{
    public:
    Type& operator[](s64 index)
    {
        BGZ_ASSERT(index < size, "Attempting to access an index which is outside of current array bounds!");
        return this->elements[index];
    };
    
    inline s64 Size() const
    {
        return size;
    };
    
    Type elements[size ? size : 1];
};

template <typename Type, s64 size>
void CopyArray(Array<Type, size> sourceArray, Array<Type, size>&& destinationArray)
{
    BGZ_ASSERT(destinationArray.Size() >= sourceArray.Size(), "Dest array needs to be at least as large as source array!");
    
    memcpy(destinationArray.elements, sourceArray.elements, sizeof(Type) * sourceArray.Size());
};