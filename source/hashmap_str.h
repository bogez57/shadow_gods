#include "dynamic_array.h"

template <typename Type>
class KeyInfo
{
public:
    i32 originalInfo{0};
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
i32 GetHashIndex(HashMap_Str<ValueType> map, const char* key)
{
    i32 indexIntoHash{};
    for(i32 i{}; key[i] != 0; ++i)
    {
        indexIntoHash += key[i];
    };

    i32 result{-1};
    if(map.keyInfos.At(indexIntoHash).originalInfo != 0)
        result = indexIntoHash;

    return result;
};

template<typename ValueType>
ValueType GetVal(HashMap_Str<ValueType> map, i32 hashIndex)
{
    return map.keyInfos.At(hashIndex).value;
};