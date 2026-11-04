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
SDL_LoadGameCode(SDL_SharedObject **GameDLL, char *GameDLLPath, game_update_and_render **GameUpdateAndRender)
{
    *GameDLL = SDL_LoadObject(GameDLLPath);
    if(*GameDLL)
    {
        *GameUpdateAndRender = (game_update_and_render *)SDL_LoadFunction(*GameDLL, "GameUpdateAndRender");
    }
}

internal void
SDL_UnloadGameCode(SDL_SharedObject **GameDLL, game_update_and_render **GameUpdateAndRender)
{
    if(*GameDLL)
    {
        SDL_UnloadObject(*GameDLL);
        *GameDLL = NULL;
    }
    *GameUpdateAndRender = NULL;
    
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL 
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
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
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    u32 AvailableSurfaceFormatsCount;
    VkSurfaceFormatKHR *AvailableSurfaceFormats;
    u32 AvailablePresentModesCount;
    VkPresentModeKHR *AvailablePresentModes;
} swapchain_support_details;



internal VkShaderModule 
LoadShaderModule(char *ShaderPath, arena *Transient, VkDevice LogicalDevice)
{
    VkShaderModule ShaderModule;
    i32 FileDescriptor;
    struct stat FileStatus;
    size_t FileSize;
    char *ShaderBytes;
    size_t BytesRead;
    ssize_t ReadResult;
    VkShaderModuleCreateInfo ShaderModuleCreateInfo;
    VkResult Result;
    
    ShaderModule = VK_NULL_HANDLE;
    
    FileDescriptor = open(ShaderPath, O_RDONLY);
    if(FileDescriptor == -1)
    {
        SDL_Log("Failed to open shader .spv file");
        return VK_NULL_HANDLE;
    }
    
    if(fstat(FileDescriptor, &FileStatus) == -1 || FileStatus.st_size <= 0 || (uintmax_t)FileStatus.st_size > SIZE_MAX || ((u64)FileStatus.st_size % sizeof(u32) != 0))
    {
        SDL_Log("Failed to get shader file status");
        return VK_NULL_HANDLE;
    }
    
    FileSize = (size_t)FileStatus.st_size;
    
    ShaderBytes = PushArray(Transient, char, FileSize);
    
    BytesRead = 0;
    while(BytesRead < FileSize)
    {
        ReadResult = read(FileDescriptor, (char *)ShaderBytes + BytesRead, FileSize - BytesRead);
        if(ReadResult <= 0)
        {
            SDL_Log("Failed to read bytes from shader .spv file");
            return VK_NULL_HANDLE;
        }
        
        BytesRead += (size_t)ReadResult;
    }
    
    close(FileDescriptor);
    
    ShaderModuleCreateInfo = (VkShaderModuleCreateInfo) 
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = FileSize,
        .pCode = (u32 *)ShaderBytes,
    };
    
    
    Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, NULL, &ShaderModule);
    if(Result != VK_SUCCESS)
    {
        SDL_Log("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    
    
    return ShaderModule;
}

