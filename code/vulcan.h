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

VkShaderModule LoadShaderModule(char *ShaderPath, arena *Transient)
{
    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    
    i32 FileDescriptor = open(ShaderPath, O_RDONLY);
    if(FileDescriptor == -1)
    {
        SDL_Log("Failed to open shader .spv file");
        return VK_NULL_HANDLE;
    }
    
    struct stat FileStatus;
    if(fstat(FileDescriptor, &FileStatus) == -1 || FileStatus.st_size <= 0 || (uintmax_t)FileStatus.st_size > SIZE_MAX || FileStatus.st_size % sizeof(u32) != 0)
    {
        SDL_Log("Failed to get shader file status");
        return VK_NULL_HANDLE;
    }
    
    size_t FileSize = (size_t)FileStatus.st_size;
    
    char *ShaderBytes = PushArray(Transient, char, FileSize);
    
    size_t BytesRead = 0;
    while(BytesRead < FileSize)
    {
        ssize_t Result = read(FileDescriptor, (char *)ShaderBytes + BytesRead, FileSize - BytesRead);
        if(Result <= 0)
        {
            SDL_Log("Failed to read bytes from shader .spv file");
            return VK_NULL_HANDLE;
        }
        
        BytesRead += (size_t)Result;
    }
    
    close(FileDescriptor);
    
    VkShaderModuleCreateInfo ShaderModuleCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = FileSize,
        .pCode = (u32 *)VertexShaderCode,
    };
    
    {
        VkResult Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, NULL, &ShaderModule);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Failed to create shader module");
            return VK_NULL_HANDLE;
        }
    }
    
    return ShaderModule;
}



