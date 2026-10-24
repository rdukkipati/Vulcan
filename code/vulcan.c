#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "game.h"
#include "vulcan.h"

SDL_AppResult SDL_AppInit(void **AppState, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    
    //=================================================================================
    //=============================INITIALIZE MEMORY===================================
    //=================================================================================
    
    memory *Memory = &State.Memory;
    Memory->PermanentStorageSize = Gigabytes(1);
    Memory->TransientStorageSize = Megabytes(64);
    
#if OS_LINUX
#include <sys/mman.h>
    
#if DEBUG
    void *BaseAddress = (void *)Terabytes(2);
    int Flag = MAP_FIXED_NOREPLACE;
#else
    void *BaseAddress = (void *)0;
    int Flag = 0;
#endif
    
    Memory->PermanentStorage = mmap(BaseAddress, Memory->PermanentStorageSize + Memory->TransientStorageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | Flag, -1, 0);
    
#elif OS_WINDOWS
    
#include <windows.h>
#if DEBUG
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = (void *)0;
#endif
    Memory->PermanentStorage = VirtualAlloc(BaseAddress, Memory->PermanentStorageSize + Memory->TransientStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
#endif
    
    Memory->TransientStorage = (u8 *)Memory->PermanentStorage + Memory->PermanentStorageSize;
    
    if(Memory->PermanentStorage == MAP_FAILED)
    {
        SDL_Log("Memory allocate failed");
        return SDL_APP_FAILURE;
    }
    
    
    //=================================================================================
    //=============================INITIALIZE SDL======================================
    //=================================================================================
    
    SDL_SetAppMetadata("Vulcan", "1.0", "com.example.vulcan");
    
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
    
    SDL_Window *Window = State.Window;
    State.WindowWidth = WINDOW_WIDTH;
    State.WindowHeight = WINDOW_HEIGHT;
    Window = SDL_CreateWindow("Vulcan", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    
#undef WINDOW_WIDTH
#undef WINDOW_HEIGHT
    
    if(!Window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    arena Arena =
    {
        .BaseAddress = State.Memory.TransientStorage,
        .RemainingSpace = State.Memory.TransientStorageSize,
    };
    
    
    
    //=================================================================================
    //====================GRAB VULKAN INSTANCE EXTENSIONS FOR SDL======================
    //=================================================================================
    
    u32 ExtensionsCount = 0;
    const char *const *Extensions_SDL = SDL_Vulkan_GetInstanceExtensions(&ExtensionsCount);
    if(!Extensions_SDL)
    {
        SDL_Log("Failed to get Vulkan Extensions for SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
#if DEBUG
    
    //=================================================================================
    //=====================DEBUG INSTANCE EXTENSIONS CHECK=============================
    //=================================================================================
    
    const char *ExtraExtensions[] = 
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        
    };
    
    u32 AvailableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableExtensionCount, NULL);
    
    VkExtensionProperties *AvailableInstanceExtensions = PushArray(&Arena, VkExtensionProperties, AvailableExtensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableExtensionCount, AvailableInstanceExtensions);
    
    b32 ExtraExtensionsAvailable = true;
    for(int i = 0; i < ArrayCount(ExtraExtensions); ++i)
    {
        b32 ExtensionFound = false;
        const char *ExtraExtension = ExtraExtensions[i];
        for(u32 j = 0; j < AvailableExtensionCount; ++j)
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
    
    
    if(!ExtraExtensionsAvailable)
    {
        SDL_Log("Extra Extensions not available");
        return SDL_APP_FAILURE;
    }
    
    
    //=================================================================================
    //========================VALIDATION LAYERS CHECK==================================
    //=================================================================================
    
    const char *ValidationLayers[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    
    u32 AvailableLayerCount;
    vkEnumerateInstanceLayerProperties(&AvailableLayerCount, NULL);
    
    // NOTE: allocate array in transient memory arena
    VkLayerProperties *AvailableLayers = PushArray(&Arena, VkLayerProperties, AvailableLayerCount);
    
    vkEnumerateInstanceLayerProperties(&AvailableLayerCount, AvailableLayers);
    
    // NOTE: Check if validation layers we request exist in list of available layers
    b32 ValidationLayersAvailable = true;
    for(int i = 0; i < ArrayCount(ValidationLayers); ++i)
    {
        b32 LayerFound = false;
        const char *ValidationLayer = ValidationLayers[i];
        for(u32 j = 0; j < AvailableLayerCount; ++j)
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
    
    if(!ValidationLayersAvailable)
    {
        SDL_Log("Validation Layers not available");
        return SDL_APP_FAILURE;
    }
    
    //=================================================================================
    //===============COMBINE SDL & DEBUG INSTANCE EXTENSIONS===========================
    //=================================================================================
    
    const char **Extensions = PushArray(&Arena, const char *, ExtensionsCount + ArrayCount(ExtraExtensions));
    
    for(u32 i = 0; i < ExtensionsCount; ++i)
    {
        Extensions[i] = Extensions_SDL[i];
    }
    
    for(u32 i = 0; i < (u32)ArrayCount(ExtraExtensions); ++i)
    {
        Extensions[ExtensionsCount + i] = ExtraExtensions[i];
    }
    
    ExtensionsCount += ArrayCount(ExtraExtensions);
    
    //=================================================================================
    //=========================VALIDATION LAYERS=======================================
    //=================================================================================
    
    u32 EnabledLayerCount = ArrayCount(ValidationLayers);
    const char *const *EnabledLayerNames = ValidationLayers;
    
    //=================================================================================
    //======================VALIDATION LAYERS DEBUG==================================== 
    //==========================CALLBACK SETUP=========================================
    //=================================================================================
    
    VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VulkanDebugCallback,
        .pUserData = NULL,
    };
    
    const void *pNext = &DebugUtilsMessengerCreateInfo;
    
#else
    
    const char **Extensions = Extensions_SDL;
    u32 EnabledLayerCount = 0;
    const char *const *EnabledLayerNames = NULL;
    
    const void *pNext = NULL;
    
#endif
    
    //=================================================================================
    //==========================VULKAN INSTANCE CREATION===============================
    //=================================================================================
    
    // NOTE: Using Vulcan 1.0 for now
    VkApplicationInfo AppInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Vulcan",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    
    VkInstanceCreateInfo CreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = pNext,
        // NOTE: flags is 0 for linux and windows. On macOS with MoltenVK, flags would be set for portability features
        .flags = 0,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = EnabledLayerCount,
        .ppEnabledLayerNames = EnabledLayerNames,
        .enabledExtensionCount = ExtensionsCount,
        .ppEnabledExtensionNames = Extensions,
    };
    
    VkInstance VulkanInstance;
    {
        VkResult Result = vkCreateInstance(&CreateInfo, NULL, &VulkanInstance);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("vkCreateInstance failed: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    //=================================================================================
    //=======================VALIDATION LAYER DEBUG CALLBACK=========================== 
    //===============================SETUP PART 2======================================
    //=================================================================================
    
#if DEBUG
    
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
    if(!CreateDebugUtilsMessenger)
    {
        SDL_Log("Loading CreateDebugUtilsMessengerEXT function failed");
        return SDL_APP_FAILURE;
    }
    
    VkDebugUtilsMessengerEXT DebugUtilsMessenger;
    {
        VkResult Result = CreateDebugUtilsMessenger(VulkanInstance, &DebugUtilsMessengerCreateInfo, NULL, &DebugUtilsMessenger);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Creating debug messenger failed: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
#endif
    
    //=================================================================================
    //==========================WINDOW SURFACE CREATION================================
    //=================================================================================
    
    VkSurfaceKHR Surface;
    if(!SDL_Vulkan_CreateSurface(Window, VulkanInstance, NULL, &Surface))
    {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    
    //=================================================================================
    //=======================PHYSICAL DEVICE ACQUISITION================================
    //=================================================================================
    
    const char *RequiredDeviceExtensions[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    queue_family_indices QueueFamilyIndices;
    ZeroStruct(QueueFamilyIndices);
    
    u32 AvailableDeviceCount = 0;
    vkEnumeratePhysicalDevices(VulkanInstance, &AvailableDeviceCount, NULL);
    if(AvailableDeviceCount == 0)
    {
        SDL_Log("Failed to find GPUs with Vulkan support!");
        return SDL_APP_FAILURE;
    }
    
    VkPhysicalDevice *AvailablePhysicalDevices = PushArray(&Arena, VkPhysicalDevice, AvailableDeviceCount);
    vkEnumeratePhysicalDevices(VulkanInstance, &AvailableDeviceCount, AvailablePhysicalDevices);
    
    for(u32 i = 0; i < AvailableDeviceCount; ++i)
    {
        
        VkPhysicalDevice AvailableDevice = AvailablePhysicalDevices[i];
        
        
        u32 AvailableDeviceExtensionsCount;
        vkEnumerateDeviceExtensionProperties(AvailableDevice, NULL, &AvailableDeviceExtensionsCount, NULL);
        VkExtensionProperties *AvailableDeviceExtensions = PushArray(&Arena, VkExtensionProperties, AvailableDeviceExtensionsCount);
        vkEnumerateDeviceExtensionProperties(AvailableDevice, NULL, &AvailableDeviceExtensionsCount, AvailableDeviceExtensions);
        
        b32 RequiredDeviceExtensionsSupported = true;
        for(u32 j = 0; j < (u32)ArrayCount(RequiredDeviceExtensions); ++j)
        {
            const char *RequiredExtension = RequiredDeviceExtensions[j];
            b32 Exists = false;
            for(u32 k = 0; k < AvailableDeviceExtensionsCount; ++k)
            {
                const char *AvailableExtension = AvailableDeviceExtensions[k].extensionName;
                if(strcmp(RequiredExtension, AvailableExtension) == 0)
                {
                    Exists = true;
                    break;
                }
                
            }
            if(!Exists)
            {
                RequiredDeviceExtensionsSupported = false;
                break;
            }
            
        }
        
        if(!RequiredDeviceExtensionsSupported)
        {
            continue;
        }
        
        
        typedef struct swapchain_support_details
        {
            VkSurfaceCapabilitiesKHR Capabilities;
            VkSurfaceFormatKHR *Formats;
            VkPresentModeKHR *PresentModes;
        } swapchain_support_details;
        
        
        swapchain_support_details SwapchainDetails;
        ZeroStruct(SwapchainDetails);
        
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(AvailableDevice, Surface, &SwapchainDetails.Capabilities);
        u32 FormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(AvailableDevice, Surface, &FormatCount, NULL);
        if(FormatCount)
        {
            SwapchainDetails.Formats = PushArray(&Arena, VkSurfaceFormatKHR, FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(AvailableDevice, Surface, &FormatCount, SwapchainDetails.Formats);
        }
        
        u32 PresentModeCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(AvailableDevice, Surface, &PresentModeCount, NULL);
        if(PresentModeCount)
        {
            SwapchainDetails.PresentModes = PushArray(&Arena, VkPresentModeKHR, PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(AvailableDevice, Surface, &PresentModeCount, SwapchainDetails.PresentModes);
        }
        
        if(SwapchainDetails.Formats == NULL || SwapchainDetails.PresentModes == NULL)
        {
            continue;
        }
        
        VkSurfaceFormatKHR ChosenFormat;
        b32 DesiredFormatFound = false;
        for(u32 j = 0; j < FormatCount; ++j)
        {
            if(SwapchainDetails.Formats[j].format == VK_FORMAT_B8G8R8A8_SRGB && SwapchainDetails.Formats[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                ChosenFormat = SwapchainDetails.Formats[j];
                DesiredFormatFound = true;
            }
        }
        if(!DesiredFormatFound)
        {
            ChosenFormat = SwapchainDetails.Formats[0];
        }
        
        VkPresentModeKHR ChosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(AvailableDevice, &DeviceProperties);
        
        VkPhysicalDeviceFeatures DeviceFeatures; vkGetPhysicalDeviceFeatures(AvailableDevice, &DeviceFeatures);
        
        queue_family_indices Temp_QueueFamilyIndices;
        ZeroStruct(Temp_QueueFamilyIndices);
        
        u32 QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(AvailableDevice, &QueueFamilyCount, NULL);
        
        VkQueueFamilyProperties *AvailableQueueFamilies = PushArray(&Arena, VkQueueFamilyProperties, QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(AvailableDevice, &QueueFamilyCount, AvailableQueueFamilies);
        b32 QueueFamiliesSupported = false;
        b32 Graphics = false;
        b32 Present = false;
        for(u32 j = 0; j < QueueFamilyCount; ++j)
        {
            VkQueueFamilyProperties *QueueFamily = &AvailableQueueFamilies[j];
            if(QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Temp_QueueFamilyIndices.GraphicsFamily.HasValue = true;
                Temp_QueueFamilyIndices.GraphicsFamily.Value = j;
                Graphics = true;
            }
            
            VkBool32 PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(AvailableDevice, j, Surface, &PresentSupport);
            if(PresentSupport)
            {
                Temp_QueueFamilyIndices.PresentFamily.HasValue = true;
                Temp_QueueFamilyIndices.PresentFamily.Value = j;
                Present = true;
            }
            
            if(Graphics && Present)
            {
                QueueFamiliesSupported = true;
                break;
            }
        }
        
        
        if(QueueFamiliesSupported)
        {
            PhysicalDevice = AvailableDevice;
            QueueFamilyIndices = Temp_QueueFamilyIndices;
            break;
        }
        
    }
    
    
    
    if(PhysicalDevice == VK_NULL_HANDLE)
    {
        SDL_Log("Failed to find suitable GPU among availble options!");
        return SDL_APP_FAILURE;
    }
    
    //=================================================================================
    //=====================COUNTING UNIQUE QUEUE FAMILIES==============================
    //=================================================================================
    
    
    u32 UniqueQueueFamilyIndicesCount = 0;
    u32 MaximumQueueFamilyIndicesCount = ArrayCount(QueueFamilyIndices.E);
    optional_u32 UniqueQueueFamilyIndices[MaximumQueueFamilyIndicesCount];
    ZeroArray(UniqueQueueFamilyIndices, MaximumQueueFamilyIndicesCount);
    
    // NOTE: Create array of unique queue family indices. Loop through our struct's indices for each family and add them to array if they don't exist in the array. O(n^2) style. We end up with a count of how many unique indices there are and an array of those unique indices.
    for(u32 i = 0; i < MaximumQueueFamilyIndicesCount; ++i)
    {
        optional_u32 Family = QueueFamilyIndices.E[i];
        for(u32 j = 0; j < MaximumQueueFamilyIndicesCount; ++j)
        {
            optional_u32 *Unique = &UniqueQueueFamilyIndices[j];
            if(!(Unique->HasValue))
            {
                ++UniqueQueueFamilyIndicesCount;
                Unique->HasValue = true;
                Unique->Value = Family.Value;
                break;
            }
            else
            {
                if(Unique->Value == Family.Value)
                {
                    break;
                }
                else
                {
                    continue;
                }
                
            }
        }
    }
    
    Assert(UniqueQueueFamilyIndicesCount <= MaximumQueueFamilyIndicesCount);
    
    //=================================================================================
    //======================UNIQUE QUEUE CREATE INFOS==================================
    //=================================================================================
    
    VkDeviceQueueCreateInfo *QueueCreateInfos = PushArray(&Arena, VkDeviceQueueCreateInfo, UniqueQueueFamilyIndicesCount);
    
    VkDevice LogicalDevice;
    
    f32 QueuePriority = 1.0f;
    for(u32 i = 0; i < UniqueQueueFamilyIndicesCount; ++i)
    {
        u32 QueueFamilyIndex = UniqueQueueFamilyIndices[i].Value;
        VkDeviceQueueCreateInfo QueueCreateInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = UniqueQueueFamilyIndices[i].Value,
            .queueCount = 1,
            .pQueuePriorities = &QueuePriority,
        };
        QueueCreateInfos[i] = QueueCreateInfo;
    }
    
    //=================================================================================
    //======================LOGICAL DEVICE CREATION====================================
    //=================================================================================
    
    
    VkPhysicalDeviceFeatures LogicalDeviceFeatures;
    ZeroStruct(LogicalDeviceFeatures);
    
    VkDeviceCreateInfo LogicalDeviceCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = UniqueQueueFamilyIndicesCount,
        .pQueueCreateInfos = QueueCreateInfos,
        .enabledLayerCount = EnabledLayerCount,
        .ppEnabledLayerNames = EnabledLayerNames,
        .enabledExtensionCount = ArrayCount(RequiredDeviceExtensions),
        .ppEnabledExtensionNames = RequiredDeviceExtensions,
        .pEnabledFeatures = &LogicalDeviceFeatures,
        
    };
    
    {
        VkResult Result = vkCreateDevice(PhysicalDevice, &LogicalDeviceCreateInfo, NULL, &LogicalDevice);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Logical device creation failed");
            return SDL_APP_FAILURE;
        }
    }
    
    VkQueue GraphicsQueue;
    vkGetDeviceQueue(LogicalDevice, QueueFamilyIndices.GraphicsFamily.Value, 0, &GraphicsQueue);
    
    VkQueue PresentQueue;
    vkGetDeviceQueue(LogicalDevice, QueueFamilyIndices.PresentFamily.Value, 0, &PresentQueue);
    
    
    
    
    char *GameDLLPath = State.GameDLLPath;
    SDL_Time *LastDLLWrite = &(State.LastDLLWrite);
    
    SDL_asprintf(&GameDLLPath, "%sgame.so", SDL_GetBasePath());
    SDL_LoadGameCode();
    *LastDLLWrite = SDL_GetLastWriteTime(GameDLLPath);
    
    return SDL_APP_CONTINUE;
    
}

SDL_AppResult SDL_AppEvent(void *AppState, SDL_Event *Event)
{
    (void)AppState;
    
    if(Event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *AppState)
{
    
    (void)AppState;
    
    char *GameDLLPath = State.GameDLLPath;
    SDL_Time LastDLLWrite = State.LastDLLWrite;
    game_update_and_render *GameUpdateAndRender = State.GameUpdateAndRender;
    
    
    SDL_Time NewDLLWrite = SDL_GetLastWriteTime(GameDLLPath);
    if(NewDLLWrite != LastDLLWrite)
    {
        LastDLLWrite = NewDLLWrite;
        SDL_UnloadGameCode();
        SDL_LoadGameCode();
    }
    
    
    
    if(GameUpdateAndRender)
    {
        GameUpdateAndRender(&State.Memory, &State.Input);
    }
    
    
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *AppState, SDL_AppResult Result)
{
    (void)AppState;
    (void)Result;
}




































