typedef struct app_state
{
    SDL_Window *Window;
    i32 WindowWidth;
    i32 WindowHeight;
    
    u64 FrameStartTime;
    
    SDL_Time LastDLLWrite;
    char *GameDLLPath;
    SDL_SharedObject *GameDLL;
    
    memory Memory;
    game_input Input;
    game_state GameState;
    
    game_update_and_render *GameUpdateAndRender;
} app_state;

global_variable app_state State;

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
    char *GameDLLPath = State.GameDLLPath;
    SDL_SharedObject **GameDLL = &(State.GameDLL);
    game_update_and_render **GameUpdateAndRender = &(State.GameUpdateAndRender);
    
    *GameDLL = SDL_LoadObject(GameDLLPath);
    if(*GameDLL)
    {
        *GameUpdateAndRender = (game_update_and_render *)SDL_LoadFunction(*GameDLL, "GameUpdateAndRender");
    }
}

internal void
SDL_UnloadGameCode(void)
{
    SDL_SharedObject **GameDLL = &(State.GameDLL);
    game_update_and_render **GameUpdateAndRender = &(State.GameUpdateAndRender);
    
    if(*GameDLL)
    {
        SDL_UnloadObject(*GameDLL);
        *GameDLL = NULL;
    }
    *GameUpdateAndRender = NULL;
    
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    (void)MessageSeverity;
    (void)MessageType;
    (void)pUserData;
    
    SDL_Log("Validation Layer: %s", pCallbackData->pMessage);
    
    return VK_FALSE;
}


typedef struct optional_u32
{
    b32 HasValue;
    u32 Value;
} optional_u32;

typedef union queue_family_indices
{
    struct
    {
        optional_u32 GraphicsFamily;
        optional_u32 PresentFamily;
    };
    optional_u32 E[2];
    
} queue_family_indices;

typedef struct swapchain_support_details
{
    VkSurfaceCapabilitiesKHR Capabilities;
    VkSurfaceFormatKHR *Formats;
    VkPresentModeKHR *PresentModes;
} swapchain_support_details;



