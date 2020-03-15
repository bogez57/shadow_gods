#ifndef VARIABLE_ARRAY_H
#define VARIABLE_ARRAY_H

template <typename Type>
class VarArray
{
    VarArray() = default;
    VarArray(i64 capacity, Memory_Partition memPart)
    {
        elements = PushType(memPart, Type, capacity);
    };

    const Type operator[](i64 index) const
    {
        BGZ_ASSERT(index < capacity, "Attempting to access index %i which is out of current arround bounds - current max index allowed: %i", index, capacity - 1);
        BGZ_ASSERT(index < length, "Attempting to access index %i which hasn't been initialized yet. Current buffer length is %i", index, length);

        return this->elements[index];
    };

    inline Type& Push()
    {
        BGZ_ASSERT(length < capacity, "Attempting to access index %i which out of current arround bounds - current max index allowed: %i", length, capacity - 1);
        return (Type&)this->elements[length++];
    };

    i64 length {};
    i64 capacity {};
    Type* elements {};
};

#endif //VARIABLE_ARRAY_H