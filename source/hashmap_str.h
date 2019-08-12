#include "dynamic_array.h"

/*
    Duplicates are overwritten
*/

ui64 CombineFirst8BytesOfString(const char* key)
{
    ui64 bitStorage {};
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

    return bitStorage;
};

i32 ProduceUniqueIDForString(const char* key)
{
    i32 uniqueID{};
    for(i32 i{}; key[i] != 0; ++i)
    {
        uniqueID += key[i];
    };

    return uniqueID;
};

template <typename Type>
class KeyInfo
{
public:
    const char* string;
    ui32 uniqueID{0};
    Type value;
    KeyInfo<Type>* nextInfo;
};

template <typename ValueType>
class HashMap_Str
{
public:
    Dynam_Array<KeyInfo<ValueType>> keyInfos;
    i32 numOfCollisions;
};

template <typename ValueType>
void Init(HashMap_Str<ValueType>&& map)
{
    map.keyInfos.Init(4096, heap);
};

template <typename ValueType>
ui16 Insert(HashMap_Str<ValueType>&& map, const char* key, ValueType value)
{
	if (!strcmp(key, "left-shoulder"))
	{
		int x{ 3 };
	};

    KeyInfo<ValueType> info{};
    info.string = key;
    info.value = value;
    info.uniqueID = ProduceUniqueIDForString(key);

    BGZ_ASSERT(info.uniqueID < 0xFFFF, "Key string was too big!");

    ui16 indexIntoHashArr{};

    ui16 mask_clearNibble{0x0FFF};

    indexIntoHashArr = (ui16)info.uniqueID;
    indexIntoHashArr = indexIntoHashArr & mask_clearNibble;

    if(NOT map.keyInfos.At(indexIntoHashArr).uniqueID)
    {
        map.keyInfos.Insert(info, indexIntoHashArr);
    }
    else
    {
        KeyInfo<ValueType>** nextInfo = &map.keyInfos.At(indexIntoHashArr).nextInfo;
        while(*nextInfo)
        {
            nextInfo = &(*nextInfo)->nextInfo;
        };

        BGZ_ASSERT(map.numOfCollisions < (map.keyInfos.size - 2) / 2, "Hash Table contains too many collisions!");
        *nextInfo = &map.keyInfos.At((map.keyInfos.size - 1) - map.numOfCollisions);
        **nextInfo = info;
        ++map.numOfCollisions;
    };

    return indexIntoHashArr;
};

template <typename ValueType>
i32 GetHashIndex(HashMap_Str<ValueType> map, const char* key)
{
    ui16 uniqueKeyID = ProduceUniqueIDForString(key);

    ui16 mask_clearNibble{0x0FFF};

    i32 indexIntoHashArr{-1};

    i32 tempIndex = (ui16)uniqueKeyID;
    tempIndex = tempIndex & mask_clearNibble;

    if(map.keyInfos.At(tempIndex).uniqueID)
        indexIntoHashArr = tempIndex;
    
    return indexIntoHashArr;
};

template<typename ValueType>
ValueType GetVal(HashMap_Str<ValueType> map, i32 hashIndex, const char* key)
{
    ValueType result{};

    ui16 uniqueKeyID = ProduceUniqueIDForString(key);

    b run{true};
    KeyInfo<ValueType>* nextKey{&map.keyInfos.At(hashIndex)};
    while(run)
    {
        if(nextKey->uniqueID == uniqueKeyID)
        {
            result = nextKey->value;   
            run = false;
        }
        else
        {
            nextKey = nextKey->nextInfo;
        }
    };

    return result;
};