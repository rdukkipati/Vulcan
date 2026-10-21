internal SDL_Time
SDL_GetLastWriteTime(const char *Filename)
{
    SDL_PathInfo FileInfo = {0};
    
    if(SDL_GetPathInfo(Filename, &FileInfo))
    {
        return FileInfo.modify_time;
    }
    
    return 0;
}

internal void
SDL_LoadGameCode(void)
{
    GameDLL = SDL_LoadObject(GamePath);
    if(GameDLL)
    {
        GameUpdateAndRender = (game_update_and_render *)SDL_LoadFunction(GameDLL, "GameUpdateAndRender");
    }
}

internal void
SDL_UnloadGameCode(void)
{
    if(GameDLL)
    {
        SDL_UnloadObject(GameDLL);
        GameDLL = NULL;
    }
    GameUpdateAndRender = NULL;
    
}

internal void
InitMemory(void)
{
    Memory = (game_memory){0};
    Memory.PermanentStorageSize = Gigabytes(1);
    Memory.TransientStorageSize = Megabytes(64);
    
#if DEBUG
    void *BaseAddress = (void *)Terabytes(2);
    int Flag = MAP_FIXED_NOREPLACE;
#else
    void *BaseAddress = (void *)0;
    int Flag = 0;
#endif
    
    Memory.PermanentStorage = mmap(BaseAddress, Memory.PermanentStorageSize + Memory.TransientStorageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | Flag, -1, 0);
    
    
    
    Memory.TransientStorage = (u8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;
}
