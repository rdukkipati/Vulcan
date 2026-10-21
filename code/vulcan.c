#include "game.h"

// NOTE: Main SDL header is in game.h because I think I want the game to use some SDL features. SDL_main and SDL_vulkan are required only by the platform layer which is why they are here.
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>


#include <string.h> // NOTE: for strcmp


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480


// TODO: Move these globals into structs later
global_variable SDL_Window *Window = NULL;

#if 0
global_variable u64 LastTime = 0;
global_variable rng RNG;
#endif

global_variable SDL_Time LastWrite = {0};
global_variable char *GameDLLPath = NULL;

global_variable SDL_SharedObject *GameDLL = NULL;


global_variable game_memory Memory;
global_variable game_input Input;

game_update_and_render *GameUpdateAndRender;

// NOTE: for mmap on linux. On Windows I would need VirtualAlloc.
#include <sys/mman.h>

#include "misc.h"

internal VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    (void)MessageSeverity;
    (void)MessageType;
    (void)pUserData;
    
    SDL_Log("Validation Layer: %s", pCallbackData->pMessage);
    
    return VK_FALSE;
}

SDL_AppResult SDL_AppInit(void **AppState, int argc, char *argv[])
{
    (void)AppState;
    (void)argc;
    (void)argv;
    
    InitMemory();
    if(Memory.PermanentStorage == MAP_FAILED)
    {
        SDL_Log("Memory allocate failed");
        return SDL_APP_FAILURE;
    }
    Input = (game_input){0};
    
    SDL_SetAppMetadata("Vulcan", "1.0", "com.example.vulcan");
    
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    Window = SDL_CreateWindow("Vulcan", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    
    if(!Window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    arena Arena =
    {
        .BaseAddress = Memory.TransientStorage,
        .RemainingSpace = Memory.TransientStorageSize,
    };
    
    u32 ExtensionsCount = 0;
    const char *const *Extensions_SDL = SDL_Vulkan_GetInstanceExtensions(&ExtensionsCount);
    if(!Extensions_SDL)
    {
        SDL_Log("Failed to get Vulkan Extensions for SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
#if DEBUG
    
    // NOTE: if debug, add debug extensions and validation layers. Verify that the debug extensions and validation layers are available by enumerating available extensions and layers and looping through them
    const char *ExtraExtensions[] = 
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        
    };
    
    u32 AvailableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableExtensionCount, NULL);
    VkExtensionProperties *AvailableExtensions = (VkExtensionProperties *)PushArena(&Arena, sizeof(VkExtensionProperties) * AvailableExtensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableExtensionCount, AvailableExtensions);
    
    b32 ExtraExtensionsAvailable = true;
    for(int i = 0; i < ArrayCount(ExtraExtensions); ++i)
    {
        b32 ExtensionFound = false;
        const char *ExtraExtension = ExtraExtensions[i];
        for(u32 j = 0; j < AvailableExtensionCount; ++j)
        {
            VkExtensionProperties *AvailableExtension = &AvailableExtensions[j];
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
    
    const char *ValidationLayers[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    
    u32 AvailableLayerCount;
    vkEnumerateInstanceLayerProperties(&AvailableLayerCount, NULL);
    
    // NOTE: allocate array in transient memory arena
    VkLayerProperties *AvailableLayers = (VkLayerProperties *)PushArena(&Arena, sizeof(VkLayerProperties) * AvailableLayerCount);
    
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
    
    const char **Extensions = (const char **)PushArena(&Arena, sizeof(const char *) * (ExtensionsCount + ArrayCount(ExtraExtensions)));
    
    for(u32 i = 0; i < ExtensionsCount; ++i)
    {
        Extensions[i] = Extensions_SDL[i];
    }
    
    for(u32 i = 0; i < (u32)ArrayCount(ExtraExtensions); ++i)
    {
        Extensions[ExtensionsCount + i] = ExtraExtensions[i];
    }
    
    ExtensionsCount += ArrayCount(ExtraExtensions);
    
    u32 EnabledLayerCount = ArrayCount(ValidationLayers);
    const char *const *EnabledLayerNames = ValidationLayers;
    
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
    
#if DEBUG
    // NOTE: so overview, we create a DebugUtilsMessenger object which is a handle that manages our Debug Callback function that logs our validation layers' messages
    
    // NOTE: have to load function first because it's part of an extension
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
    
    
    
    
    // NOTE: asprintf here appends the name of the game dll to the location of the executable
    SDL_asprintf(&GameDLLPath, "%sgame.so", SDL_GetBasePath());
    SDL_LoadGameCode();
    LastWrite = SDL_GetLastWriteTime(GameDLLPath);
    
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
    
    SDL_Time NewWrite = SDL_GetLastWriteTime(GameDLLPath);
    if(NewWrite != LastWrite)
    {
        LastWrite = NewWrite;
        SDL_UnloadGameCode();
        SDL_LoadGameCode();
    }
    
    
    
    if(GameUpdateAndRender)
    {
        GameUpdateAndRender(&Memory, &Input);
    }
    
    
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *AppState, SDL_AppResult Result)
{
    (void)AppState;
    (void)Result;
}




































