#ifndef VARIABLE_ARRAY_H
#define VARIABLE_ARRAY_H

template <typename Type>
class VarArray
{
public:
    VarArray() = default;
    VarArray(Memory_Partition* memPart, i64 capacity)
    {
        this->capacity = capacity;
        this->elements = PushType(memPart, Type, capacity);
    };

    Type& operator[](i64 index)
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

template <typename Type>
void Initialize(VarArray<Type>&& varArr, Memory_Partition* memPart, i64 capacity)
{
    BGZ_ASSERT(varArr.capacity == 0, "Trying to initialize array twice!");

    varArr.capacity = capacity;
    varArr.elements = PushType(memPart, Type, capacity);
};

template <typename Type>
void CopyArray(VarArray<Type> sourceArray, VarArray<Type>&& destinationArray)
{
    BGZ_ASSERT(destinationArray.capacity == sourceArray.capacity, "Variable Array capacities do not match!");
    BGZ_ASSERT(destinationArray.elements == sourceArray.elements, "Both varialbe arrays pointing to same memory address");

    memcpy(destinationArray.elements, sourceArray.elements, sizeof(Type) * sourceArray.size);
    destinationArray.length = sourceArray.length;
};

#endif //VARIABLE_ARRAY_H