#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <sys/mman.h> // mmap
#include <fcntl.h> // open() and O_RDONLY
#include <sys/stat.h> // struct stat and fstat()
#include <unistd.h> // read() close()
#include <stddef.h> // size_t
#include <sys/types.h> // ssize_t

#include "game.h"
#include "vulcan.h"


SDL_AppResult SDL_AppInit(void **Data, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    u32 i;
    u32 j;
    u32 k;
    
    VkResult Result;
    
    //==================MEMORY=========================
    memory Memory;
    void *BaseAddress;
    int Flag;
    arena Eternal;
    
    app_state *AppState;
    arena Transient;
    
    //================SDL WINDOW=======================
    SDL_Window *Window;
    
    //================VULKAN INSTANCE==================
    VkInstance VulkanInstance;
    const char *ExtraInstanceExtensionsNames[] = 
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        
    };
    const char *RequiredValidationLayersNames[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    u32 InstanceExtensionsCount;
    const char *const *InstanceExtensions_SDL;
    u32 AvailableInstanceExtensionsCount;
    VkExtensionProperties *AvailableInstanceExtensions;
    b32 ExtraInstanceExtensionsAreAvailable;
    b32 ExtraInstanceExtensionFound;
    const char *ExtraInstanceExtensionName;
    const char *AvailableExtensionName;
    u32 AvailableLayersCount;
    VkLayerProperties *AvailableLayers;
    b32 RequiredValidationLayersAreAvailable;
    b32 RequiredLayerFound;
    const char *RequiredValidationLayerName;
    const char *AvailableLayerName;
    const char **InstanceExtensions;
    u32 EnabledLayersCount;
    const char *const *EnabledLayersNames;
    VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo;
    const void *pNext;
    VkApplicationInfo AppInfo;
    VkInstanceCreateInfo VulkanInstanceCreateInfo;
    
    //================DEBUG MESSENGER==================
    VkDebugUtilsMessengerEXT DebugUtilsMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessenger; 
    
    //===================SURFACE=======================
    VkSurfaceKHR Surface;
    
    //================PHYSICAL DEVICE==================
    VkPhysicalDevice PhysicalDevice;
    const char *RequiredDeviceExtensionsNames[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    queue_family_indices QueueFamilyIndices;
    u32 AvailablePhysicalDevicesCount;
    VkPhysicalDevice *AvailablePhysicalDevices;
    VkPhysicalDevice AvailablePhysicalDevice;
    u32 AvailableDeviceExtensionsCount;
    VkExtensionProperties *AvailableDeviceExtensions;
    b32 RequiredDeviceExtensionsAreSupported;
    const char *RequiredDeviceExtensionName;
    b32 RequiredDeviceExtensionExists;
    const char *AvailableDeviceExtensionName;
    swapchain_support_details SwapchainSupportDetails;
    queue_family_indices Temp_QueueFamilyIndices;
    u32 AvailableQueueFamiliesCount;
    VkQueueFamilyProperties *AvailableQueueFamilies;
    b32 RequiredQueueFamiliesAreSupported;
    b32 GraphicsFamilyExists;
    b32 PresentFamilyExists;
    VkQueueFamilyProperties *AvailableQueueFamily;
    VkBool32 PresentFamilySupported;
    
    //================LOGICAL DEVICE===================
    VkDevice LogicalDevice;
    u32 MaximumQueueFamilyIndicesCount = ArrayCount(QueueFamilyIndices.E);
    optional_u32 UniqueQueueFamilyIndices[MaximumQueueFamilyIndicesCount]; 
    optional_u32 Family;
    optional_u32 *Unique;
    u32 UniqueQueueFamilyIndicesCount;
    f32 QueuePriority;
    VkDeviceQueueCreateInfo *QueueCreateInfos;
    VkPhysicalDeviceFeatures LogicalDeviceFeatures;
    VkDeviceCreateInfo LogicalDeviceCreateInfo;
    
    //==================SWAPCHAIN======================
    VkSwapchainKHR Swapchain;
    b32 DesiredFormatFound;
    VkSurfaceFormatKHR SwapchainImageFormat;
    VkPresentModeKHR SwapchainPresentMode;
    VkExtent2D SwapchainExtent;
    u32 MinimumSwapchainImageCount;
    VkSharingMode SwapchainImageSharingMode;
    u32 SwapchainQueueFamilyIndexCount;
    u32 *SwapchainQueueFamilyIndices;
    VkSwapchainCreateInfoKHR SwapchainCreateInfo;
    
    
    //================IMAGE VIEWS======================
    VkImageView *SwapchainImageViews;
    u32 SwapchainImageCount;
    VkImage *SwapchainImages;
    VkComponentMapping Components;
    VkImageSubresourceRange SubresourceRange;
    VkImageViewCreateInfo ImageViewCreateInfo;
    
    
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //=============================INITIALIZE MEMORY===================================
    //=================================================================================
    //=================================================================================
    Memory = (memory)
    {
        .PermanentStorageSize = Gigabytes(1),
        .PermanentStorage = NULL,
        .TransientStorageSize = Megabytes(64),
        .TransientStorage = NULL,
    };
    
#if DEBUG
    BaseAddress = (void *)Terabytes(2);
    Flag = MAP_FIXED_NOREPLACE;
#else
    BaseAddress = (void *)0;
    Flag = 0;
#endif
    
    Memory.PermanentStorage = mmap(BaseAddress, Memory.PermanentStorageSize + Memory.TransientStorageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | Flag, -1, 0);
    
    Memory.TransientStorage = (u8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;
    
    if(Memory.PermanentStorage == MAP_FAILED)
    {
        SDL_Log("Memory allocate failed");
        return SDL_APP_FAILURE;
    }
    
    Eternal = (arena) 
    {
        .BaseAddress = Memory.PermanentStorage,
        .RemainingSpace = Memory.PermanentStorageSize,
    };
    
    AppState = PushStruct(&Eternal, app_state);
    AppState->Memory = Memory;
    AppState->Eternal = Eternal;
    
    *Data = AppState;
    
    Transient = (arena)
    {
        .BaseAddress = Memory.TransientStorage,
        .RemainingSpace = Memory.TransientStorageSize,
    };
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //=========================INITIALIZE SDL & WINDOW=================================
    //=================================================================================
    //=================================================================================
    
    SDL_SetAppMetadata("Vulcan", "1.0", "com.example.vulcan");
    
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
    
    Window = SDL_CreateWindow("Vulcan", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    
#undef WINDOW_WIDTH
#undef WINDOW_HEIGHT
    
    if(!Window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //==========================VULKAN INSTANCE CREATION===============================
    //=================================================================================
    //=================================================================================
    
    
    //=======================GRAB SDL EXTENSIONS FOR VULKAN============================
    InstanceExtensionsCount = 0;
    InstanceExtensions_SDL = SDL_Vulkan_GetInstanceExtensions(&InstanceExtensionsCount);
    if(!InstanceExtensions_SDL)
    {
        SDL_Log("Failed to get Vulkan Extensions for SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    
    
#if DEBUG
    
    //==================VERIFY EXTRA EXTENSIONS AVAILABLE==============================
    AvailableInstanceExtensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableInstanceExtensionsCount, NULL);
    
    AvailableInstanceExtensions = PushArray(&Transient, VkExtensionProperties, AvailableInstanceExtensionsCount);
    vkEnumerateInstanceExtensionProperties(NULL, &AvailableInstanceExtensionsCount, AvailableInstanceExtensions);
    
    ExtraInstanceExtensionsAreAvailable = true;
    for(i = 0; i < (u32)ArrayCount(ExtraInstanceExtensionsNames); ++i)
    {
        ExtraInstanceExtensionFound = false;
        ExtraInstanceExtensionName = ExtraInstanceExtensionsNames[i];
        for(j = 0; j < AvailableInstanceExtensionsCount; ++j)
        {
            AvailableExtensionName = AvailableInstanceExtensions[j].extensionName;
            if(strcmp(ExtraInstanceExtensionName, AvailableExtensionName) == 0)
            {
                ExtraInstanceExtensionFound = true;
                break;
            }
        }
        if(!ExtraInstanceExtensionFound)
        {
            ExtraInstanceExtensionsAreAvailable = false;
            break;
        }
    }
    
    if(!ExtraInstanceExtensionsAreAvailable)
    {
        SDL_Log("Extra Extensions not available");
        return SDL_APP_FAILURE;
    }
    
    //==================VERIFY VALIDATION LAYERS AVAILABLE=============================
    AvailableLayersCount = 0;
    vkEnumerateInstanceLayerProperties(&AvailableLayersCount, NULL);
    
    AvailableLayers = PushArray(&Transient, VkLayerProperties, AvailableLayersCount);
    vkEnumerateInstanceLayerProperties(&AvailableLayersCount, AvailableLayers);
    
    RequiredValidationLayersAreAvailable = true;
    for(i = 0; i < (u32)ArrayCount(RequiredValidationLayersNames); ++i)
    {
        RequiredLayerFound = false;
        RequiredValidationLayerName = RequiredValidationLayersNames[i];
        for(j = 0; j < AvailableLayersCount; ++j)
        {
            AvailableLayerName = AvailableLayers[j].layerName;
            if(strcmp(RequiredValidationLayerName, AvailableLayerName) == 0)
            {
                RequiredLayerFound = true;
                break;
            }
        }
        if(!RequiredLayerFound)
        {
            RequiredValidationLayersAreAvailable = false;
            break;
        }
    }
    
    if(!RequiredValidationLayersAreAvailable)
    {
        SDL_Log("Validation Layers not available");
        return SDL_APP_FAILURE;
    }
    
    //==================COMBINE SDL & EXTRA EXTENSIONS=================================
    InstanceExtensions = PushArray(&Transient, const char *, InstanceExtensionsCount + ArrayCount(ExtraInstanceExtensionsNames));
    
    for(i = 0; i < InstanceExtensionsCount; ++i)
    {
        InstanceExtensions[i] = InstanceExtensions_SDL[i];
    }
    
    for(i = 0; i < (u32)ArrayCount(ExtraInstanceExtensionsNames); ++i)
    {
        InstanceExtensions[InstanceExtensionsCount + i] = ExtraInstanceExtensions[i];
    }
    
    InstanceExtensionsCount += ArrayCount(ExtraInstanceExtensionsNames);
    
    //==================SETUP OTHER VULKAN INSTANCE====================================
    //========================CREATE INFO STUFF========================================
    EnabledLayersCount = ArrayCount(RequiredValidationLayersNames);
    EnabledLayersNames = RequiredValidationLayersNames;
    
    DebugUtilsMessengerCreateInfo = (VkDebugUtilsMessengerCreateInfoEXT) 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VulkanDebugCallback,
        .pUserData = NULL,
    };
    
    pNext = &DebugUtilsMessengerCreateInfo;
    
#else
    
    //==================NO VALIDATION LAYERS OR DEBUG EXTENSIONS=======================
    InstanceExtensions = Extensions_SDL;
    EnabledLayersCount = 0;
    EnabledLayersNames = NULL;
    
    pNext = NULL;
    
#endif
    
    //==========================CREATE VULKAN INSTANCE=================================
    AppInfo = (VkApplicationInfo)
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Vulcan",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    
    
    VulkanInstanceCreateInfo = (VkInstanceCreateInfo) 
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = pNext,
        .flags = 0,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = EnabledLayersCount,
        .ppEnabledLayerNames = EnabledLayersNames,
        .enabledExtensionCount = InstanceExtensionsCount,
        .ppEnabledExtensionNames = InstanceExtensions,
    };
    
    
    
    Result = vkCreateInstance(&VulkanInstanceCreateInfo, NULL, &VulkanInstance);
    if(Result != VK_SUCCESS)
    {
        SDL_Log("vkCreateInstance failed: %d", Result);
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //==========================CREATE DEBUG MESSENGER=================================
    //=================================================================================
    //=================================================================================
#if DEBUG
    
    CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
    if(!CreateDebugUtilsMessenger)
    {
        SDL_Log("Loading CreateDebugUtilsMessengerEXT function failed");
        return SDL_APP_FAILURE;
    }
    
    Result = CreateDebugUtilsMessenger(VulkanInstance, &DebugUtilsMessengerCreateInfo, NULL, &DebugUtilsMessenger);
    if(Result != VK_SUCCESS)
    {
        SDL_Log("Creating debug messenger failed: %d", Result);
        return SDL_APP_FAILURE;
    };
    
#endif
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //==========================WINDOW SURFACE CREATION================================
    //=================================================================================
    //=================================================================================
    if(!SDL_Vulkan_CreateSurface(Window, VulkanInstance, NULL, &Surface))
    {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //=========================PHYSICAL DEVICE SELECTION===============================
    //=================================================================================
    //=================================================================================
    
    ZeroStruct(QueueFamilyIndices);
    
    AvailablePhysicalDevicesCount = 0;
    vkEnumeratePhysicalDevices(VulkanInstance, &AvailablePhysicalDevicesCount, NULL);
    if(AvailablePhysicalDevicesCount == 0)
    {
        SDL_Log("Failed to find GPUs with Vulkan support!");
        return SDL_APP_FAILURE;
    }
    
    AvailablePhysicalDevices = PushArray(&Transient, VkPhysicalDevice, AvailablePhysicalDevicesCount);
    vkEnumeratePhysicalDevices(VulkanInstance, &AvailablePhysicalDevicesCount, AvailablePhysicalDevices);
    
    //======================CHOOSE PHYSICAL DEVICE=====================================
    PhysicalDevice = VK_NULL_HANDLE;
    
    for(i = 0; i < AvailablePhysicalDevicesCount; ++i)
    {
        //========================VERIFY REQUIRED PHYSICAL DEVICE==========================
        //===========================EXTENSIONS ARE AVAILABLE==============================
        AvailablePhysicalDevice = AvailablePhysicalDevices[i];
        AvailableDeviceExtensionsCount = 0;
        AvailableDeviceExtensions = NULL;
        
        vkEnumerateDeviceExtensionProperties(AvailablePhysicalDevice, NULL, &AvailableDeviceExtensionsCount, NULL);
        AvailableDeviceExtensions = PushArray(&Transient, VkExtensionProperties, AvailableDeviceExtensionsCount);
        vkEnumerateDeviceExtensionProperties(AvailablePhysicalDevice, NULL, &AvailableDeviceExtensionsCount, AvailableDeviceExtensions);
        
        RequiredDeviceExtensionsAreSupported = true;
        for(j = 0; j < (u32)ArrayCount(RequiredDeviceExtensionsNames); ++j)
        {
            RequiredDeviceExtensionName = RequiredDeviceExtensionsNames[j];
            RequiredDeviceExtensionExists = false;
            for(k = 0; k < AvailableDeviceExtensionsCount; ++k)
            {
                AvailableDeviceExtensionName = AvailableDeviceExtensions[k].extensionName;
                if(strcmp(RequiredDeviceExtensionName, AvailableDeviceExtensionName) == 0)
                {
                    RequiredDeviceExtensionExists = true;
                    break;
                }
            }
            if(!RequiredDeviceExtensionsExists)
            {
                RequiredDeviceExtensionsAreSupported = false;
                break;
            }
        }
        
        if(!RequiredDeviceExtensionsAreSupported)
        {
            continue;
        }
        
        //========================CHECK SWAPCHAIN SUPPORT==================================
        ZeroStruct(SwapchainSupportDetails);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails.SurfaceCapabilities);
        vkGetPhysicalDeviceSurfaceFormatsKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails.AvailableSurfaceFormatsCount, NULL);
        if(SwapchainSupportDetails.AvailableSurfaceFormatsCount)
        {
            SwapchainSupportDetails.AvailableSurfaceFormats = PushArray(&Transient, VkSurfaceFormatKHR, SwapchainSupportDetails.AvailableSurfaceFormatsCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails.AvailableSurfaceFormatsCount, SwapchainSupportDetails.AvailableSurfaceFormats);
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails.AvailablePresentModesCount, NULL);
        if(SwapchainSupportDetails.AvailablePresentModesCount)
        {
            SwapchainSupportDetails.AvailablePresentModes = PushArray(&Transient, VkPresentModeKHR, SwapchainSupportDetails.AvailablePresentModesCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(AvailablePhysicalDevice, Surface, &SwapchainSupportDetails.AvailablePresentModesCount, SwapchainSupportDetails.AvailablePresentModes);
        }
        if(SwapchainSupportDetails.AvailableSurfaceFormats == NULL || SwapchainSupportDetails.AvailablePresentModes == NULL)
        {
            continue;
        }
        
        //========================CHECK FOR REQUIRED=======================================
        //==========================QUEUE FAMILIES=========================================
        ZeroStruct(Temp_QueueFamilyIndices);
        
        AvailableQueueFamiliesCount = 0;
        
        vkGetPhysicalDeviceQueueFamilyProperties(AvailablePhysicalDevice, &AvailableQueueFamiliesCount, NULL);
        AvailableQueueFamilies = PushArray(&Transient, VkQueueFamilyProperties, AvailableQueueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(AvailablePhysicalDevice, &AvailableQueueFamiliesCount, AvailableQueueFamilies);
        
        RequiredQueueFamiliesAreSupported = false;
        GraphicsFamilyExists = false;
        PresentFamilyExists = false;
        for(j = 0; j < AvailableQueueFamiliesCount; ++j)
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
                RequiredQueueFamiliesAreSupported = true;
                break;
            }
        }
        
        if(RequiredQueueFamiliesAreSupported)
        {
            PhysicalDevice = AvailablePhysicalDevice;
            QueueFamilyIndices = Temp_QueueFamilyIndices;
            break;
        }
    }
    
    if(PhysicalDevice == VK_NULL_HANDLE)
    {
        SDL_Log("Failed to find suitable GPU among available options!");
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //=========================LOGICAL DEVICE CREATION=================================
    //=================================================================================
    //=================================================================================
    
    //=====================COUNT UNIQUE QUEUE FAMILIES=================================
    ZeroArray(UniqueQueueFamilyIndices, MaximumQueueFamilyIndicesCount);
    
    UniqueQueueFamilyIndicesCount = 0;
    for(i = 0; i < MaximumQueueFamilyIndicesCount; ++i)
    {
        Family = QueueFamilyIndices.E[i];
        for(j = 0; j < MaximumQueueFamilyIndicesCount; ++j)
        {
            Unique = &UniqueQueueFamilyIndices[j];
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
    
    //======================UNIQUE QUEUE CREATE INFOS==================================
    QueueCreateInfos = PushArray(&Transient, VkDeviceQueueCreateInfo, UniqueQueueFamilyIndicesCount);
    
    QueuePriority = 1.0f;
    for(i = 0; i < UniqueQueueFamilyIndicesCount; ++i)
    {
        QueueCreateInfos[i] = (VkDeviceQueueCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = UniqueQueueFamilyIndices[i].Value,
            .queueCount = 1,
            .pQueuePriorities = &QueuePriority,
        };
    }
    
    //======================LOGICAL DEVICE CREATION====================================
    ZeroStruct(LogicalDeviceFeatures);
    
    LogicalDeviceCreateInfo = (VkDeviceCreateInfo)
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = UniqueQueueFamilyIndicesCount,
        .pQueueCreateInfos = QueueCreateInfos,
        .enabledLayerCount = EnabledLayersCount,
        .ppEnabledLayerNames = EnabledLayersNames,
        .enabledExtensionCount = ArrayCount(RequiredDeviceExtensionsNames),
        .ppEnabledExtensionNames = RequiredDeviceExtensionsNames,
        .pEnabledFeatures = &LogicalDeviceFeatures,
        
    };
    
    Result = vkCreateDevice(PhysicalDevice, &LogicalDeviceCreateInfo, NULL, &LogicalDevice);
    if(Result != VK_SUCCESS)
    {
        SDL_Log("Logical device creation failed");
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //============================CREATE SWAPCHAIN=====================================
    //================================================================================= 
    //================================================================================= 
    
    //======================CHOOSE SWAPCHAIN IMAGE FORMAT==============================
    DesiredFormatFound = false;
    for(i = 0; i < SwapchainSupportDetails.AvailableSurfaceFormatsCount; ++i)
    {
        if(SwapchainSupportDetails.AvailableSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && SwapchainSupportDetails.AvailableSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            SwapchainImageFormat = SwapchainSupportDetails.AvailableSurfaceFormats[i];
            DesiredFormatFound = true;
        }
    }
    if(!DesiredFormatFound)
    {
        SwapchainImageFormat = SwapchainSupportDetails.AvailableSurfaceFormats[0];
    }
    
    
    
    //======================CHOOSE SWAPCHAIN PRESENT MODE==============================
    SwapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    //=========================CHOOSE SWAPCHAIN EXTENT=================================
    if(SwapchainSupportDetails.SurfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        SwapchainExtent = SwapchainSupportDetails.SurfaceCapabilities.currentExtent;
    }
    else
    {
        i32 Width;
        i32 Height;
        VkExtent2D Extent;
        if(!SDL_GetWindowSizeInPixels(Window, &Width, &Height))
        {
            SDL_Log("Failed to get window size in pixels: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
        Extent = (VkExtent2D) 
        {
            .width = (u32)width,
            .height = (u32)height,
        };
        
        Extent.width = Clamp(Extent.width, SwapchainSupportDetails.SurfaceCapabilities.minImageExtent.width, SwapchainSupportDetails.SurfaceCapabilities.maxImageExtent.width);
        Extent.height = Clamp(Extent.height, SwapchainSupportDetails.SurfaceCapabilities.minImageExtent.height, SwapchainSupportDetails.SurfaceCapabilities.maxImageExtent.height);
        
        SwapchainExtent = Extent;
    }
    
    //==================CHOOSE MINIMUM SWAPCHAIN IMAGE COUNT===========================
    MinimumSwapchainImageCount = SwapchainSupportDetails.SurfaceCapabilities.minImageCount;
    
    if(SwapchainSupportDetails.SurfaceCapabilities.maxImageCount > 0 && MinimumSwapchainImageCount > SwapchainSupportDetails.SurfaceCapabilities.maxImageCount)
    {
        MinimumSwapchainImageCount = SwapchainSupportDetails.SurfaceCapabilities.maxImageCount;
    }
    
    Assert(*MinimumSwapchainImageCount == 2);
    
    
    //==================CHOOSE SWAPCHAIN IMAGE SHARING MODE============================
    if(QueueFamilyIndices.GraphicsFamily.Value != QueueFamilyIndices.PresentFamily.Value)
    {
        SwapchainImageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapchainQueueFamilyIndexCount = 2;
        SwapchainQueueFamilyIndices = PushArray(&Transient, u32, 2);
        SwapchainQueueFamilyIndices[0] = QueueFamilyIndices.GraphicsFamily.Value;
        SwapchainQueueFamilyIndices[1] = QueueFamilyIndices.PresentFamily.Value;
    }
    else
    {
        SwapchainImageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapchainQueueFamilyIndexCount = 0;
        SwapchainQueueFamilyIndices = NULL;
    }
    
    
    
    //=========================CREATE SWAPCHAIN========================================
    SwapchainCreateInfo = (VkSwapchainCreateInfoKHR)
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR ,
        .pNext = NULL,
        .flags = 0,
        .surface = Surface,
        .minImageCount = MinimumSwapchainImageCount,
        .imageFormat = SwapchainImageFormat.format,
        .imageColorSpace = SwapchainImageFormat.colorSpace,
        .imageExtent = SwapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = SwapchainImageSharingMode,
        .queueFamilyIndexCount = SwapchainQueueFamilyIndexCount,
        .pQueueFamilyIndices = SwapchainQueueFamilyIndices,
        .preTransform = SwapchainSupportDetails.SurfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = SwapchainPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    
    
    Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, NULL, &Swapchain);
    if(Result != VK_SUCCESS)
    {
        SDL_Log("Swapchain creation failed: %d", Result);
        return SDL_APP_FAILURE;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    //=================================================================================
    //=================================================================================
    //===========================CREATE IMAGE VIEWS====================================
    //=================================================================================
    //=================================================================================
    SwapchainImageCount = 0;
    vkGetSwapchainImagesKHR(LogicalDevice, Swapchain, &SwapchainImageCount, NULL);
    Assert(SwapchainImageCount == 2);
    SwapchainImages = PushArray(Transient, VkImage, SwapchainImageCount);
    vkGetSwapchainImagesKHR(LogicalDevice, Swapchain, &SwapchainImageCount, SwapchainImages);
    
    SwapchainImageViews = PushArray(&AppState->Eternal, VkImageView, SwapchainImageCount);
    
    Components = (VkComponentMapping)
    {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    
    SubresourceRange = (VkImageSubresourceRange) 
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    
    for(i = 0; i < SwapchainImageCount; ++i)
    {
        ImageViewCreateInfo = (VkImageViewCreateInfo) 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = SwapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = SwapchainImageFormat.format,
            .components = Components,
            .subresourceRange = SubresourceRange,
        };
        
        
        Result = vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, NULL, &SwapchainImageViews[i]);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Failed to create image view: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    char *VertexShaderPath;
    char *FragmentShaderPath;
    SDL_asprintf(&VertexShaderPath, "%svert.spv", SDL_GetBasePath());
    SDL_asprintf(&FragmentShaderPath, "%sfrag.spv", SDL_GetBasePath());
    
    VkShaderModule VertexShaderModule = LoadShaderModule(VertexShaderPath, &Transient, LogicalDevice);
    VkShaderModule FragmentShaderModule = LoadShaderModule(FragmentShaderPath, &Transient, LogicalDevice);
    
    if(VertexShaderModule == VK_NULL_HANDLE || FragmentShaderModule == VK_NULL_HANDLE)
    {
        SDL_Log("Failed to create create shader module");
        return SDL_APP_FAILURE;
    }
    
    VkPipelineShaderStageCreateInfo VertexShaderStageCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = VertexShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL,
        
    };
    
    VkPipelineShaderStageCreateInfo FragmentShaderStageCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = FragmentShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL,
        
    };
    
    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertexShaderStageCreateInfo, FragmentShaderStageCreateInfo };
    
    VkPipelineVertexInputStateCreateInfo VertexInputCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };
    
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    
    VkViewport Viewport = 
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)SwapchainExtent.width,
        .height = (f32)SwapchainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D Scissor = 
    {
        .offset = { 0, 0 },
        .extent = SwapchainExtent,
    };
    
    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &Viewport,
        .scissorCount = 1,
        .pScissors = &Scissor,
    };
    
    VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
        
    };
    
    VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    
    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = 
    {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    
    
    VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &ColorBlendAttachmentState,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL,
    };
    
    VkPipelineLayout PipelineLayout;
    
    {
        VkResult Result = vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, NULL, &PipelineLayout);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Failed to create pipeline layout: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    VkAttachmentDescription ColorAttachmentDescription = 
    {
        .flags = 0,
        .format = SwapchainImageFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    
    VkAttachmentReference ColorAttachmentReference = 
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    
    VkSubpassDescription SubpassDescription = 
    {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ColorAttachmentReference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = NULL,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    
    VkRenderPass RenderPass;
    
    VkRenderPassCreateInfo RenderPassCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &ColorAttachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &SubpassDescription,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };
    
    {
        VkResult Result = vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, NULL, &RenderPass);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Failed to create render pass: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount = 2,
        .pStages = ShaderStages,
        .pVertexInputState = &VertexInputCreateInfo,
        .pInputAssemblyState = &InputAssemblyStateCreateInfo,
        .pTessellationState = NULL,
        .pViewportState = &ViewportStateCreateInfo,
        .pRasterizationState = &RasterizationStateCreateInfo,
        .pMultisampleState = &MultisampleStateCreateInfo,
        .pDepthStencilState = NULL,
        .pColorBlendState = &ColorBlendStateCreateInfo,
        .pDynamicState = NULL,
        .layout = PipelineLayout,
        .renderPass = RenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
        
    };
    
    VkPipeline GraphicsPipeline;
    {
        VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo, NULL, &GraphicsPipeline);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Failed to create graphics pipeline: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    
    
    VkQueue GraphicsQueue;
    vkGetDeviceQueue(LogicalDevice, QueueFamilyIndices.GraphicsFamily.Value, 0, &GraphicsQueue);
    
    VkQueue PresentQueue;
    vkGetDeviceQueue(LogicalDevice, QueueFamilyIndices.PresentFamily.Value, 0, &PresentQueue);
    
    
    
    SDL_asprintf(&AppState->GameDLLPath, "%sgame.so", SDL_GetBasePath());
    SDL_LoadGameCode((void *)AppState);
    AppState->LastGameDLLWrite = SDL_GetLastWriteTime(AppState->GameDLLPath);
    
    return SDL_APP_CONTINUE;
    
}

SDL_AppResult SDL_AppEvent(void *Data, SDL_Event *Event)
{
    (void)Data;
    
    if(Event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *Data)
{
    app_state *AppState = (app_state *)Data;
    
    
    SDL_Time NewGameDLLWrite = SDL_GetLastWriteTime(AppState->GameDLLPath);
    if(NewGameDLLWrite != AppState->LastGameDLLWrite)
    {
        AppState->LastGameDLLWrite = NewGameDLLWrite;
        SDL_UnloadGameCode(Data);
        SDL_LoadGameCode(Data);
    }
    
    
    
    if(AppState->GameUpdateAndRender)
    {
        AppState->GameUpdateAndRender(&AppState->&AppState->Eternal, &AppState->Memory, &AppState->Input, &AppState->GameState);
    }
    
    
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *Data, SDL_AppResult Result)
{
    (void)Data;
    (void)Result;
}




































