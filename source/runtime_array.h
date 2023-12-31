#ifndef VARIABLE_ARRAY_H
#define VARIABLE_ARRAY_H

template <typename Type>
struct RunTimeArr
{
    public:
    RunTimeArr() = default;
    RunTimeArr(Memory_Partition* memPart, s64 capacity)
    {
        this->capacity = capacity;
        this->elements = PushType(memPart, Type, capacity);
    };
    
    Type& operator[](s64 index)
    {
        BGZ_ASSERT(index < capacity, "Attempting to access an index which is outside of current arround bounds!");
        BGZ_ASSERT(index < length, "Attempting to access an index which hasn't been initialized yet!");
        
        return this->elements[index];
    };
    
    inline Type& Push(Type element = Type())
    {
        BGZ_ASSERT(length < capacity, "Trying to add more elements to array than array has capacity for!");
        return (Type&)this->elements[length++] = element;
    };
    
    s64 length {};
    s64 capacity {};
    Type* elements {};
};

template <typename Type>
void InitArr(RunTimeArr<Type>&& varArr, Memory_Partition* memPart, s64 capacity)
{
    BGZ_ASSERT(varArr.capacity == 0, "Trying to initialize array twice!");
    
    varArr.capacity = capacity;
    varArr.elements = PushType(memPart, Type, capacity);
};

template <typename Type>
void CopyArray(RunTimeArr<Type> sourceArray, RunTimeArr<Type>&& destinationArray)
{
    BGZ_ASSERT(destinationArray.capacity == sourceArray.capacity, "Variable Array capacities do not match!");
    BGZ_ASSERT(destinationArray.elements != sourceArray.elements, "Both varialbe arrays pointing to same memory address!");
    
    memcpy(destinationArray.elements, sourceArray.elements, sizeof(Type) * sourceArray.length);
    destinationArray.length = sourceArray.length;
};

#endif //VARIABLE_ARRAY_H