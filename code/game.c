#include "game.h"

internal void *
PushArena(arena *Arena, u64 Size)
{
    Assert(Arena->RemainingSpace >= Size);
    void *Result = Arena->BaseAddress;
    Arena->BaseAddress += Size;
    return Result;
}

extern GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    
}