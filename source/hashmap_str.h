#include "dynamic_array.h"

template <typename Type>
class KeyInfo
{
public:
    i32 originalInfo;
    Type value;
};

template <typename ValueType>
class HashMap_Str
{
public:
    Dynam_Array<KeyInfo<ValueType>> keyInfos;
};

template <typename ValueType>
void Init(HashMap_Str<ValueType>&& map)
{
    map.keyInfos.Init(4096, heap);
};

template <typename ValueType>
void Insert(HashMap_Str<ValueType>&& map, const char* key, ValueType value)
{
    i32 indexIntoHash{};
    for(i32 i{}; key[i] != 0; ++i)
    {
        char singleChar = key[i];
        indexIntoHash += singleChar;
    };

    KeyInfo<ValueType> info{};
    info.originalInfo = indexIntoHash;
    info.value = value;
    map.keyInfos.Insert(info, indexIntoHash);
};

template <typename ValueType>
ValueType Get(HashMap_Str<ValueType> map, const char* key)
{
    ValueType result{};

    i32 indexIntoHash{};
    for(i32 i{}; key[i] != 0; ++i)
    {
        indexIntoHash += key[i];
    };

    result = map.keyInfos.At(indexIntoHash).value;
    return result;
};