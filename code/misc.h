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
SDL_LoadGameCode(app_state *AppState)
{
    char *GameDLLPath = *AppState.GameDLLPath;
    SDL_SharedObject **GameDLL = &(*AppState.GameDLL);
    game_update_and_render **GameUpdateAndRender = &(*AppState.GameUpdateAndRender);
    
    *GameDLL = SDL_LoadObject(GameDLLPath);
    if(*GameDLL)
    {
        *GameUpdateAndRender = (game_update_and_render *)SDL_LoadFunction(*GameDLL, "GameUpdateAndRender");
    }
}

internal void
SDL_UnloadGameCode(app_state *AppState)
{
    SDL_SharedObject **GameDLL = &(*AppState.GameDLL);
    game_update_and_render **GameUpdateAndRender = &(*AppState.GameUpdateAndRender);
    
    if(*GameDLL)
    {
        SDL_UnloadObject(*GameDLL);
        *GameDLL = NULL;
    }
    *GameUpdateAndRender = NULL;
    
}

