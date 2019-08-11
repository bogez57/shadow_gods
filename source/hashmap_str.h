#include "dynamic_array.h"

template <typename Type>
class KeyInfo
{
public:
    ui8 uniqueID{0};
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
    map.keyInfos.Init(2048, heap);
};

template <typename ValueType>
void Insert(HashMap_Str<ValueType>&& map, const char* key, ValueType value)
{
    KeyInfo<ValueType> info{};
    info.value = value;

    ui64 bitStorage {};
    {//Pack bits of string together. Only take first 8 bytes of string and dump rest
        i32 numTimesToIterate{};
        for(i32 i{}; key[i] != 0; ++i)
            ++numTimesToIterate;

        if(numTimesToIterate > 8)
            numTimesToIterate = 8;

        ui8* pointerToBitStorage {(ui8*)&bitStorage};
        for(i32 i{}; i < numTimesToIterate; ++i)
        {
            char singleChar = key[i];
            *pointerToBitStorage = singleChar;
            pointerToBitStorage += 1;
        };
    };

    ui16 indexIntoHashArr{};
    ui16 mask{0x0FFF};
    for(i32 i{}; i < 2; ++i)
    {
        indexIntoHashArr = (ui16)bitStorage;
        indexIntoHashArr = indexIntoHashArr & mask;
    };
};

template <typename ValueType>
i32 GetHashIndex(HashMap_Str<ValueType> map, const char* key)
{
    i32 hashIndex{};
    return hashIndex;
};

template<typename ValueType>
ValueType GetVal(HashMap_Str<ValueType> map, i32 hashIndex)
{
    return map.keyInfos.At(hashIndex).value;
};