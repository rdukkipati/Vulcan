typedef struct app_state
{
    
    VkDevice LogicalDevice;
    
    
    u64 FrameStartTime;
    
    SDL_Time LastGameDLLWrite;
    char *GameDLLPath;
    SDL_SharedObject *GameDLL;
    
    memory Memory;
    game_input Input;
    game_state GameState;
    
    arena Eternal;
    
    game_update_and_render *GameUpdateAndRender;
} app_state;


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
SDL_LoadGameCode(void *Data)
{
    app_state *AppState = (app_state *)Data;
    
    AppState->GameDLL = SDL_LoadObject(AppState->GameDLLPath);
    if(AppState->GameDLL)
    {
        AppState->GameUpdateAndRender = (game_update_and_render *)SDL_LoadFunction(AppState->GameDLL, "GameUpdateAndRender");
    }
}

internal void
SDL_UnloadGameCode(void *Data)
{
    app_state *AppState = (app_state *)Data;
    
    if(AppState->GameDLL)
    {
        SDL_UnloadObject(AppState->GameDLL);
        AppState->GameDLL = NULL;
    }
    AppState->GameUpdateAndRender = NULL;
    
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
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    VkSurfaceFormatKHR *AvailableSurfaceFormats;
    VkPresentModeKHR *AvailablePresentModes;
} swapchain_support_details;

VkShaderModule LoadShaderModule(char *ShaderPath, arena *Transient, VkDevice LogicalDevice)
{
    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    
    i32 FileDescriptor = open(ShaderPath, O_RDONLY);
    if(FileDescriptor == -1)
    {
        SDL_Log("Failed to open shader .spv file");
        return VK_NULL_HANDLE;
    }
    
    struct stat FileStatus;
    if(fstat(FileDescriptor, &FileStatus) == -1 || FileStatus.st_size <= 0 || (uintmax_t)FileStatus.st_size > SIZE_MAX || ((u64)FileStatus.st_size % sizeof(u32) != 0))
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
        .pCode = (u32 *)ShaderBytes,
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

internal b32
VerifyExtraExtensionsAvailable(const char *ExtraInstanceExtensions, u32 AvailableInstanceExtensionsCount, VkExtensionProperties *AvailableInstanceExtensions)
{
    b32 ExtraInstanceExtensionsAvailable = true;
    u32 i;
    for(i = 0; i < (u32)ArrayCount(ExtraInstanceExtensions); ++i)
    {
        b32 InstanceExtensionFound = false;
        const char *ExtraInstanceExtension = ExtraInstanceExtensions[i];
        u32 j;
        for(j = 0; j < AvailableInstanceExtensionsCount; ++j)
        {
            VkExtensionProperties *AvailableExtension = &AvailableInstanceExtensions[j];
            if(strcmp(ExtraExtension, AvailableExtension->extensionName) == 0)
            {
                ExtensionFound = true;
                break;
            }
        }
        if(!ExtensionFound)
        {
            ExtraExtensionsAvailable = false;
            break;
        }
    }
    
    return ExtraInstanceExtensionsAvailable;
    
}

internal b32
VerifyValidationLayersAvailable(const char *ValidationLayers, u32 AvailableLayersCount, VkLayerProperties *AvailableLayers)
{
    
    b32 ValidationLayersAvailable = true;
    u32 i;
    for(i = 0; i < (u32)ArrayCount(ValidationLayers); ++i)
    {
        
        b32 LayerFound = false;
        const char *ValidationLayer = ValidationLayers[i];
        u32 j;
        
        for(j = 0; j < AvailableLayersCount; ++j)
        {
            
            VkLayerProperties *AvailableLayer = &AvailableLayers[j];
            if(strcmp(ValidationLayer, AvailableLayer->layerName) == 0)
            {
                LayerFound = true;
                break;
            }
            
        }
        if(!LayerFound)
        {
            ValidationLayersAvailable = false;
            break;
        }
    }
    
    return ValidationLayersAvailable;
    
}

internal void
ChoosePhysicalDevice(u32 AvailablePhysicalDevicesCount, VkPhysicalDevice *AvailablePhysicalDevices, arena *Transient, const char **RequiredPhysicalDeviceExtensionsNames, swapchain_support_details *SwapchainSupportDetails, VkSurfaceKHR Surface, u32 *AvailableSurfaceFormatsCount, u32 *AvailablePresentModesCount, VkPhysicalDevice *PhysicalDevice, queue_family_indices *QueueFamilyIndices)
{
    u32 i;
    u32 j;
    u32 k;
    
    VkPhysicalDevice AvailablePhysicalDevice;
    u32 AvailablePhysicalDeviceExtensionsCount;
    VkExtensionProperties *AvailablePhysicalDeviceExtensions;
    
    b32 RequiredPhysicalDeviceExtensionsSupported;
    const char *RequiredPhysicalDeviceExtensionName;
    b32 RequiredPhysicalDeviceExtensionExists;
    const char *AvailablePhysicalDeviceExtensionName;
    
    queue_family_indices Temp_QueueFamilyIndices;
    u32 AvailableQueueFamiliesCount;
    
    b32 RequiredQueueFamiliesSupported;
    b32 GraphicsFamilyExists;
    b32 PresentFamilyExists;
    
    VkQueueFamilyProperties *AvailableQueueFamily;
    VkBool32 PresentFamilySupported;
    
    
    *PhysicalDevice = VK_NULL_HANDLE;
    
    for(i = 0; i < AvailablePhysicalDevicesCount; ++i)
    {
        //=================================================================================
        //========================VERIFY REQUIRED PHYSICAL DEVICE==========================
        //===========================EXTENSIONS ARE AVAILABLE==============================
        //=================================================================================
        AvailablePhysicalDevice = AvailablePhysicalDevices[i];
        AvailablePhysicalDeviceExtensionsCount = 0;
        AvailablePhysicalDeviceExtensions = NULL;
        
        vkEnumerateDeviceExtensionProperties(AvailableDevice, NULL, &AvailablePhysicalDeviceExtensionsCount, NULL);
        AvailablePhysicalDeviceExtensions = PushArray(Transient, VkExtensionProperties, AvailablePhysicalDeviceExtensionsCount);
        vkEnumerateDeviceExtensionProperties(AvailableDevice, NULL, &AvailablePhysicalDeviceExtensionsCount, AvailablePhysicalDeviceExtensions);
        
        RequiredPhysicalDeviceExtensionsSupported = true;
        
        for(j = 0; j < (u32)ArrayCount(RequiredPhysicalDeviceExtensionsNames); ++j)
        {
            
            RequiredPhysicalDeviceExtensionName = RequiredPhysicalDeviceExtensionsNames[j];
            RequiredPhysicalDeviceExtensionExists = false;
            
            for(k = 0; k < AvailablePhysicalDeviceExtensionsCount; ++k)
            {
                
                AvailablePhysicalDeviceExtensionName = AvailablePhysicalDeviceExtensions[k].extensionName;
                
                if(strcmp(RequiredPhysicalDeviceExtensionName, AvailablePhysicalDeviceExtensionName) == 0)
                {
                    
                    RequiredPhysicalDeviceExtensionExists = true;
                    break;
                    
                }
                
            }
            
            if(!RequiredPhysicalDeviceExtensionsExists)
            {
                
                RequiredPhysicalDeviceExtensionsSupported = false;
                break;
                
            }
            
        }
        
        if(!RequiredDeviceExtensionsSupported)
        {
            continue;
        }
        
        
        //=================================================================================
        //========================CHECK SWAPCHAIN SUPPORT==================================
        //=================================================================================
        ZeroStruct(*SwapchainSupportDetails);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails->SurfaceCapabilities);
        *AvailableSurfaceFormatsCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(AvailablePhysicalDevice, Surface, AvailableSurfaceFormatsCount, NULL);
        if(*AvailableSurfaceFormatsCount)
        {
            SwapchainSupportDetails->AvailableSurfaceFormats = PushArray(Transient, VkSurfaceFormatKHR, *AvailableSurfaceFormatsCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(AvailablePhysicalDevice, Surface, AvailableSurfaceFormatsCount, SwapchainSupportDetails->AvailableSurfaceFormats);
        }
        
        *AvailablePresentModesCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(AvailablePhysicalDevice, Surface, AvailablePresentModesCount, NULL);
        if(*AvailablePresentModesCount)
        {
            SwapchainSupportDetails->AvailablePresentModes = PushArray(Transient, VkPresentModeKHR, *AvailablePresentModesCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(AvailablePhysicalDevice, Surface, AvailablePresentModesCount, SwapchainSupportDetails->AvailablePresentModes);
        }
        
        if(SwapchainSupportDetails->AvailableSurfaceFormats == NULL || SwapchainSupportDetails->AvailablePresentModes == NULL)
        {
            continue;
        }
        
        /*
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(AvailableDevice, &DeviceProperties);
        
        VkPhysicalDeviceFeatures DeviceFeatures; vkGetPhysicalDeviceFeatures(AvailableDevice, &DeviceFeatures);
        */
        
        //===========================================================================
        //========================CHECK FOR REQUIRED=================================
        //==========================QUEUE FAMILIES===================================
        //===========================================================================
        ZeroStruct(Temp_QueueFamilyIndices);
        
        AvailableQueueFamiliesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(AvailablePhysicalDevice, &AvailableQueueFamiliesCount, NULL);
        
        VkQueueFamilyProperties *AvailableQueueFamilies = PushArray(&Transient, VkQueueFamilyProperties, QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(AvailableDevice, &QueueFamilyCount, AvailableQueueFamilies);
        RequiredQueueFamiliesSupported = false;
        GraphicsFamilyExists = false;
        PresentFamilyExists = false;
        
        for(j = 0; j < QueueFamilyCount; ++j)
        {
            
            AvailableQueueFamily = &AvailableQueueFamilies[j];
            if(AvailableQueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                
                Temp_QueueFamilyIndices.GraphicsFamily.HasValue = true;
                Temp_QueueFamilyIndices.GraphicsFamily.Value = j;
                GraphicsFamilyExists = true;
                
            }
            
            PresentFamilySupported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(AvailablePhysicalDevice, j, Surface, &PresentFamilySupported);
            
            if(PresentFamilySupported)
            {
                
                Temp_QueueFamilyIndices.PresentFamily.HasValue = true;
                Temp_QueueFamilyIndices.PresentFamily.Value = j;
                PresentFamilyExists = true;
                
            }
            
            if(GraphicsFamilyExists && PresentFamilyExists)
            {
                
                RequiredQueueFamiliesSupported = true;
                break;
                
            }
        }
        
        
        if(RequiredQueueFamiliesSupported)
        {
            
            *PhysicalDevice = AvailablePhysicalDevice;
            *QueueFamilyIndices = Temp_QueueFamilyIndices;
            
            break;
            
        }
        
    }
    
}


