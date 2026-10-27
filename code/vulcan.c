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
    
    
#if DEBUG
    void *BaseAddress = (void *)Terabytes(2);
    int Flag = MAP_FIXED_NOREPLACE;
#else
    void *BaseAddress = (void *)0;
    int Flag = 0;
#endif
    
    Memory->PermanentStorage = mmap(BaseAddress, Memory->PermanentStorageSize + Memory->TransientStorageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | Flag, -1, 0);
    
    
    
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
    
    
    
    swapchain_support_details SwapchainDetails;
    u32 FormatCount;
    u32 PresentModeCount;
    
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
        
        
        
        ZeroStruct(SwapchainDetails);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(AvailableDevice, Surface, &SwapchainDetails.Capabilities);
        FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(AvailableDevice, Surface, &FormatCount, NULL);
        if(FormatCount)
        {
            SwapchainDetails.Formats = PushArray(&Arena, VkSurfaceFormatKHR, FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(AvailableDevice, Surface, &FormatCount, SwapchainDetails.Formats);
        }
        
        PresentModeCount = 0;
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
    
    VkExtent2D SwapchainExtent;
    VkSurfaceFormatKHR ChosenSurfaceFormat;
    VkPresentModeKHR ChosenPresentMode;
    
    
    b32 DesiredFormatFound = false;
    for(u32 j = 0; j < FormatCount; ++j)
    {
        if(SwapchainDetails.Formats[j].format == VK_FORMAT_B8G8R8A8_SRGB && SwapchainDetails.Formats[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            ChosenSurfaceFormat = SwapchainDetails.Formats[j];
            DesiredFormatFound = true;
        }
    }
    if(!DesiredFormatFound)
    {
        ChosenSurfaceFormat = SwapchainDetails.Formats[0];
    }
    
    ChosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    
    if(SwapchainDetails.Capabilities.currentExtent.width != UINT32_MAX)
    {
        SwapchainExtent = SwapchainDetails.Capabilities.currentExtent;
    }
    else
    {
        int width, height;
        if(!SDL_GetWindowSizeInPixels(Window, &width, &height))
        {
            SDL_Log("Failed to get window size in pixels: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
        VkExtent2D Extent = 
        {
            .width = (u32)width,
            .height = (u32)height,
        };
        
        Extent.width = Clamp(Extent.width, SwapchainDetails.Capabilities.minImageExtent.width, SwapchainDetails.Capabilities.maxImageExtent.width);
        Extent.height = Clamp(Extent.height, SwapchainDetails.Capabilities.minImageExtent.height, SwapchainDetails.Capabilities.maxImageExtent.height);
        
        SwapchainExtent = Extent;
    }
    
    u32 SwapchainImageCount = SwapchainDetails.Capabilities.minImageCount;
    if(SwapchainImageCount <= 2)
    {
        SwapchainImageCount = 2;
    }
    if(SwapchainDetails.Capabilities.maxImageCount > 0 && SwapchainImageCount > SwapchainDetails.Capabilities.maxImageCount)
    {
        SwapchainImageCount = SwapchainDetails.Capabilities.maxImageCount;
    }
    
    Assert(SwapchainImageCount == 2);
    
    VkSharingMode ImageSharingMode;
    u32 QueueFamilyIndexCount;
    const u32 *pQueueFamilyIndices;
    u32 QueueFamilyIndexValues[] = {QueueFamilyIndices.GraphicsFamily.Value, QueueFamilyIndices.PresentFamily.Value};
    if(QueueFamilyIndices.GraphicsFamily.Value != QueueFamilyIndices.PresentFamily.Value)
    {
        ImageSharingMode = VK_SHARING_MODE_CONCURRENT;
        QueueFamilyIndexCount = 2;
        pQueueFamilyIndices = QueueFamilyIndexValues;
    }
    else
    {
        ImageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        QueueFamilyIndexCount = 0;
        pQueueFamilyIndices = NULL;
    }
    
    VkSwapchainCreateInfoKHR SwapchainCreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR ,
        .pNext = NULL,
        .flags = 0,
        .surface = Surface,
        .minImageCount = SwapchainImageCount,
        .imageFormat = ChosenSurfaceFormat.format,
        .imageColorSpace = ChosenSurfaceFormat.colorSpace,
        .imageExtent = SwapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = ImageSharingMode,
        .queueFamilyIndexCount = QueueFamilyIndexCount,
        .pQueueFamilyIndices = pQueueFamilyIndices,
        .preTransform = SwapchainDetails.Capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = ChosenPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
        
    };
    
    
    
    
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
    
    VkSwapchainKHR Swapchain;
    
    {
        VkResult Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, NULL, &Swapchain);
        if(Result != VK_SUCCESS)
        {
            SDL_Log("Swapchain creation failed: %d", Result);
            return SDL_APP_FAILURE;
        }
    }
    
    SwapchainImageCount = 0;
    vkGetSwapchainImagesKHR(LogicalDevice, Swapchain, &SwapchainImageCount, NULL);
    VkImage *SwapchainImages = PushArray(&Eternal, VkImage, SwapchainImageCount);
    vkGetSwapchainImagesKHR(LogicalDevice, Swapchain, &SwapchainImageCount, SwapchainImages);
    
    VkImageView *SwapchainImageViews = PushArray(&Eternal, VkImageView, SwapchainImageCount);
    
    VkComponentMapping Components =
    {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    
    VkImageSubresourceRange SubresourceRange = 
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    
    for(int i = 0; i < SwapchainImageCount; ++i)
    {
        VkImageViewCreateInfo ImageViewCreateInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = SwapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ChosenSurfaceFormat,
            .components = Components,
            .subresourceRange = SubresourceRange,
        };
        VkResult Result = vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, NULL, SwapchainImageViews[i]);
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
    
    VkShaderModule VertexShaderModule = LoadShaderModule(VertexShaderPath, &Transient);
    VkShaderModule FragmentShaderModule = LoadShaderModule(FragmentShaderPath, &Transient);
    
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
    
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = 
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




































