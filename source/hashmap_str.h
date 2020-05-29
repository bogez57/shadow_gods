#include "dynamic_array.h"

/*
    Duplicates are overwritten
*/

#if 0
enum Error
{
    HASH_DOES_NOT_EXIST = -1
};

template <typename Type>
class KeyInfo
{
    public:
    const char* originalString{nullptr};
    s32 uniqueID{};
    Type value;
    KeyInfo<Type>* nextInfo;
};

template <typename ValueType>
class HashMap_Str
{
    public:
    HashMap_Str() = default;
    HashMap_Str(s32 memParitionID) :
    keyInfos{4096, memParitionID}
    {};
    
    Dynam_Array<KeyInfo<ValueType>> keyInfos;
    s32 numOfCollisions{};
};

template <typename ValueType>
HashMap_Str<ValueType> CopyHashMap(HashMap_Str<ValueType> src)
{
    HashMap_Str<ValueType> dest = src;
    
    CopyArray(src.keyInfos, $(dest.keyInfos));
    
    return dest;
};

s32 _ProduceUniqueIDForString(const char* key)
{
    s32 uniqueID{};
    for (s32 i{}; key[i] != 0; ++i)
    {
        uniqueID += key[i];
    };
    
    return uniqueID;
};

u16 _HashFunction(s32 numberToCondense)
{
    BGZ_ASSERT(numberToCondense < 0xFFFF, "Key originalString was too big!");
    
    u16 indexIntoHashArr{};
    u16 mask_clearNibble{ 0x0FFF };
    
    indexIntoHashArr = (u16)numberToCondense;
    indexIntoHashArr = indexIntoHashArr & mask_clearNibble;
    
    return indexIntoHashArr;
};

template <typename ValueType>
void CleanUpHashMap_Str(HashMap_Str<ValueType>&& map)
{
    map.numOfCollisions = 0;
    CleanUpDynamArr($(map.keyInfos));
};

template <typename ValueType>
u16 Insert(HashMap_Str<ValueType>&& map, const char* key, ValueType value)
{
    KeyInfo<ValueType> info{};
    info.originalString = key;
    info.value = value;
    info.uniqueID = _ProduceUniqueIDForString(key);
    
    u16 indexIntoHashArr = _HashFunction(info.uniqueID);
    
    if (NOT map.keyInfos.At(indexIntoHashArr).uniqueID || map.keyInfos.At(indexIntoHashArr).uniqueID == info.uniqueID)
    {
        Insert($(map.keyInfos), info, indexIntoHashArr);
    }
    else
    {
        KeyInfo<ValueType>** nextInfo = &map.keyInfos.At(indexIntoHashArr).nextInfo;
        
        while (*nextInfo)
        {
            if ((*nextInfo)->uniqueID == info.uniqueID)
                break;
            else
                nextInfo = &(*nextInfo)->nextInfo;
        };
        
        BGZ_ASSERT(map.numOfCollisions < (map.keyInfos.size - 2) / 2, "Hash Table contains too many collisions! Reached end of space allocated for collision entries");
        
        *nextInfo = &map.keyInfos.At((map.keyInfos.size - 1) - map.numOfCollisions);
        **nextInfo = info;
        ++map.numOfCollisions;
    };
    
    return indexIntoHashArr;
};

template <typename ValueType>
s32 GetHashIndex(HashMap_Str<ValueType> map, const char* key)
{
    u16 uniqueKeyID = _ProduceUniqueIDForString(key);
    
    u16 mask_clearNibble{ 0x0FFF };
    
    s32 indexIntoHashArr{ Error::HASH_DOES_NOT_EXIST };
    
    s32 tempIndex = _HashFunction(uniqueKeyID);
    if (map.keyInfos.At(tempIndex).uniqueID)
        indexIntoHashArr = tempIndex;
    
    return indexIntoHashArr;
};

template <typename ValueType>
ValueType* GetVal(HashMap_Str<ValueType> map, s32 hashIndex, const char* key)
{
    ValueType* result{};
    
    u16 uniqueKeyID = _ProduceUniqueIDForString(key);
    
    bool run{ true };
    KeyInfo<ValueType>* nextKey{ &map.keyInfos.At(hashIndex) };
    while (run)
    {
        if (nextKey->uniqueID == uniqueKeyID)
        {
            result = &nextKey->value;
            run = false;
        }
        else
        {
            nextKey = nextKey->nextInfo;
        }
    };
    
    return result;
};
#endif