#pragma once
#include "types.h"

struct Game_Memory
{
    bool IsInitialized{false};

    uint32 PermanentStorageSize{};
    void* PermanentStorage{nullptr};
    uint64 TemporaryStorageSize{};
    void* TemporaryStorage{nullptr};
};

struct Game_Input
{

};

struct Game_Sound_Output_Buffer
{

};

struct Game_Render_Cmd_Buffer
{

};

struct Read_File_Result {void* FileContents{nullptr}; uint32 FileContentsSize{};};
struct Platform_Services
{
    Read_File_Result (*ReadEntireFile)(const char*);
    bool (*WriteEntireFile)(const char*, void*, uint32);
    void (*FreeFileMemory)(void*);
};