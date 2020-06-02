#ifndef DEBUG_ARRAY_H
#define DEBUG_ARRAY_H

template <typename Type, s64 capacity = 1>
class DbgArray
{
    public:
    Type& operator[](s64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which is out of current arround bounds - current max index allowed: %i", index, capacity - 1);
        BGZ_ASSERT(index < length, "Attempting to access index %i which hasn't been initialized yet. Current buffer length is %i", index, length);
        
        return this->elements[index];
    };
    
    inline Type& Push(Type element = Type())
    {
        BGZ_ASSERT(length < capacity, "Attempting to access index %i which out of current arround bounds - current max index allowed: %i", length, capacity - 1);
        return (Type&)this->elements[length++];
    };
    
    s64 length {};
    Type elements[capacity ? capacity : 1];
};

#endif //DEBUG_ARRAY_H
