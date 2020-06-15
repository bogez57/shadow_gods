#ifndef DEBUG_ARRAY_H
#define DEBUG_ARRAY_H

template <typename Type, s64 capacity = 1>
class DbgArray
{
    public:
    Type& operator[](s64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access an index which is outside of current arround bounds!");
        BGZ_ASSERT(index < length, "Attempting to access an index which hasn't been initialized yet!");
        
        return this->elements[index];
    };
    
    inline Type& Push(Type element = Type())
    {
        BGZ_ASSERT(length < capacity, "Attempting to push too many elements onto array!");
        return (Type&)this->elements[length++];
    };
    
    s64 length {};
    Type elements[capacity ? capacity : 1];
};

#endif //DEBUG_ARRAY_H
