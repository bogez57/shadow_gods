
struct Game_Memory
{
    bool IsInitialized { false };

    ui32 SizeOfPermanentStorage {};
    void* PermanentStorage { nullptr };

    ui64 SizeOfTemporaryStorage {};
    ui64 TemporaryStorageUsed {};
    void* TemporaryStorage { nullptr };
    ui64 TotalSize {};
};

void* _MallocType(i64);
void* _CallocType(i64);
void* _ReAlloc(void*, i64);
void _DeAlloc(void**);

void InitDynamAllocator();
void CreateRegionFromGameMem_1(Game_Memory* GameMemory, i64 size);

#define MallocType(Type, Count) (Type*)_MallocType(((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType((Size))
#define CallocType(Type, Count) (Type*)_CallocType(((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType((Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc((void**)&PtrToMemory)
