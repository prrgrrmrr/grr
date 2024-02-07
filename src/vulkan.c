#include "vulkan.h"

VkInstance instance;                      // Vulkan instance
VkSurfaceKHR surface;                     // Surface to present to
VkPhysicalDevice physicalDevice;          // Physical device
VkDevice device;                          // Interface to physical device
GrrQueueFamilyIndices queueFamilyIndices; // Indices of queue families
VkQueue graphicsQueue;                    // Graphics queue handle
VkQueue presentQueue;                     // Presentation queue handle
VkQueue computeQueue;                     // Compute queue handle
VkSwapchainKHR swapchain;                 // Swap chain
VkSurfaceFormatKHR selectedFormat;        // Swap chain format
VkExtent2D selectedExtent;                // Swap chain extent
VkPresentModeKHR selectedPresentMode;     // Swap chain present mode
Grr_u32 imageCount;                       // Image count
VkImage *swapchainImages;                 // Images created by swapchain
VkImageView *imageViews;                  // Views to swapchain images
VkFramebuffer *swapchainFramebuffers;     // Frame buffers

Grr_bool recreateSwapChain = false;
Grr_bool windowMinimized = false;

// Pipeline
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

// Commands
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;

// Sync objects
VkSemaphore *imageAvailableSemaphores;
VkSemaphore *renderFinishedSemaphores;
VkFence *inFlightFences;

// Buffer and buffer memory
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
VkBuffer *uniformBuffers;
VkDeviceMemory *uniformBuffersMemory;
void **uniformBuffersMapped;

// Descriptor set layout
VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSetLayout *layouts;
VkDescriptorSet *descriptorSets;

// Depth
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

// Texture
VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;

// Model
GrrModel model;

const Grr_u32 MAX_FRAMES_IN_FLIGHT = 2;
Grr_u32 currentFrame = 0;

#if defined(GRR_DEBUG)
VkDebugUtilsMessengerEXT debugMessenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL Grr_vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  const Grr_string msg = (const Grr_string)pCallbackData->pMessage;

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    GRR_LOG_INFO("%s\n", msg);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    GRR_LOG_WARNING("%s\n", msg);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    GRR_LOG_ERROR("%s\n", msg);
    break;
  default:
    GRR_LOG_DEBUG("%s\n", msg);
    break;
  }

  return VK_TRUE;
}
#endif

void _Grr_destroyVulkanInstance() {
  GRR_LOG_INFO("Free vulkan instance\n");
#if defined(GRR_DEBUG)
  PFN_vkDestroyDebugUtilsMessengerEXT fpDestroyDebugUtilsMessengerEXT =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (fpDestroyDebugUtilsMessengerEXT)
    fpDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
#endif
  vkDestroyInstance(instance, NULL);
}

bool _Grr_createVulkanInstance() {
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = windowInfo->title;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if defined(GRR_PLATFORM_MACOS)
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
  createInfo.pApplicationInfo = &appInfo;

  // Eextension names
  const Grr_string extensionNames[] = {
#if defined(GRR_DEBUG)
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif

    VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(GRR_PLATFORM_MACOS)
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_EXT_METAL_SURFACE_EXTENSION_NAME
#endif
  };

  // Extension count
  Grr_u32 extensionCount = 1;

#if defined(GRR_DEBUG)
  extensionCount += 1;
#endif

#if defined(GRR_PLATFORM_MACOS)
  extensionCount += 3;
#endif

  GRR_LOG_DEBUG("Extensions\n");
  for (Grr_u32 i = 0; i < extensionCount; i++) {
    GRR_LOG_DEBUG("\t%s\n", extensionNames[i]);
  }

  // Check extension names are available
  Grr_u32 availableExtensionCount;
  vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL);
  VkExtensionProperties *pExtensionProperties = (VkExtensionProperties *)malloc(
      sizeof(VkExtensionProperties) * availableExtensionCount);
  vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount,
                                         pExtensionProperties);

  Grr_bool extensionsOk = true;
  for (Grr_i32 i = 0; i < extensionCount && extensionsOk; i++) {
    Grr_bool found = false;
    for (Grr_i32 j = 0; j < availableExtensionCount && !found; j++) {
      if (strcmp(pExtensionProperties[j].extensionName, extensionNames[i]) == 0)
        found = true;
    }
    if (!found) {
      GRR_LOG_CRITICAL("Required extension %s not available\n",
                       extensionNames[i]);
      extensionsOk = false;
    }
  }
  free(pExtensionProperties);
  if (!extensionsOk) {
    return false;
  }

  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = (const char *const *)extensionNames;

  // Validation layer names
#if defined(GRR_DEBUG)
  const Grr_string validationLayerNames[] = {"VK_LAYER_KHRONOS_validation"};
  Grr_u32 layerCount = 1;

  // Check validation layer names are available
  Grr_u32 availableLayerCount;
  vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
  VkLayerProperties *pLayerProperties = (VkLayerProperties *)malloc(
      sizeof(VkLayerProperties) * availableLayerCount);
  vkEnumerateInstanceLayerProperties(&availableLayerCount, pLayerProperties);

  Grr_bool layersOk = true;
  for (Grr_i32 i = 0; i < layerCount && layersOk; i++) {
    Grr_bool layerFound = false;
    for (Grr_i32 j = 0; j < availableLayerCount && !layerFound; j++) {
      if (strcmp(pLayerProperties[j].layerName, validationLayerNames[i]) == 0)
        layerFound = true;
    }
    if (!layerFound) {
      GRR_LOG_CRITICAL("Required validation layer %s not available\n",
                       validationLayerNames[i]);
      layersOk = false;
    }
  }
  free(pLayerProperties);
  if (!layersOk) {
    return false;
  }

  createInfo.enabledLayerCount = layerCount;
  createInfo.ppEnabledLayerNames = (const char *const *)validationLayerNames;

  // Khronos: To capture events that occur while creating or destroying an
  // instance, an application can link a VkDebugReportCallbackCreateInfoEXT
  // structure or a VkDebugUtilsMessengerCreateInfoEXT structure to the pNext
  // element of the VkInstanceCreateInfo structure given to vkCreateInstance

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
  debugCreateInfo.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugCreateInfo.messageSeverity =
      // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = Grr_vulkanDebugCallback;
  debugCreateInfo.pUserData = NULL;

  createInfo.pNext = &debugCreateInfo;
#endif

  if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
    return false;

#if defined(GRR_DEBUG)
  PFN_vkCreateDebugUtilsMessengerEXT fpCreateDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (fpCreateDebugUtilsMessengerEXT) {
    if (fpCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, NULL,
                                       &debugMessenger) != VK_SUCCESS)
      return false;
  } else
    return false;
#endif

  atexit(_Grr_destroyVulkanInstance);
  return true;
}

Grr_bool _Grr_deviceMeetsRequirements(VkPhysicalDevice device) {
  // TODO: check device meets minimum rquirements
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
          properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
         supportedFeatures.samplerAnisotropy;
}

Grr_bool _Grr_selectPhysicalDevice() {
  Grr_u32 deviceCount;
  vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
  if (deviceCount == 0) {
    GRR_LOG_CRITICAL("No physical device available\n");
    return false;
  }
  VkPhysicalDevice *physicalDevices =
      (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);
  Grr_bool foundDevice = false;
  for (Grr_u32 i = 0; i < deviceCount; i++) {
    if (_Grr_deviceMeetsRequirements(physicalDevices[i])) {
      foundDevice = true;
      physicalDevice = physicalDevices[i];
      break;
    }
  }
  free(physicalDevices);
  if (!foundDevice) {
    GRR_LOG_CRITICAL("No physical device meets requirements\n");
    return false;
  }

  return true;
}

void _Grr_destroyLogicalDevice() {
  GRR_LOG_INFO("Free logical device\n");
  vkDestroyDevice(device, NULL);
}

Grr_bool _Grr_createLogicalDevice() {

  if (queueFamilyIndices.graphicsFamilyIndex == -1 ||
      queueFamilyIndices.presentFamilyIndex == -1) {
    GRR_LOG_CRITICAL("No support for graphics or present family queues\n");
    return false;
  }

  // Queue infos
  Grr_u32 queueIndices[] = {queueFamilyIndices.graphicsFamilyIndex,
                            queueFamilyIndices.computeFamilyIndex,
                            queueFamilyIndices.transferFamilyIndex,
                            queueFamilyIndices.sparseBndingFamilyIndex,
                            queueFamilyIndices.presentFamilyIndex};
  Grr_u32 maxQueueIndices = sizeof(queueIndices) / sizeof(Grr_u32);

  // Find unique queue indices
  Grr_u32 uniqueQueueIndices[maxQueueIndices];
  Grr_u32 uniqueCount = 0;
  for (Grr_u32 i = 0; i < maxQueueIndices; i++) {
    if (queueIndices[i] != -1) {
      Grr_bool exists = false;
      for (Grr_u32 j = 0; j < uniqueCount && !exists; j++) {
        if (uniqueQueueIndices[j] == queueIndices[i])
          exists = true;
      }
      if (!exists)
        uniqueQueueIndices[uniqueCount++] = queueIndices[i];
    }
  }

  // Create unique queues
  VkDeviceQueueCreateInfo queueCreateInfos[uniqueCount];
  Grr_f32 queuePriority = 1.0;
  for (Grr_u32 i = 0; i < uniqueCount; i++) {
    queueCreateInfos[i] = (VkDeviceQueueCreateInfo){0};
    queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[i].queueFamilyIndex = uniqueQueueIndices[i];
    queueCreateInfos[i].queueCount = 1;
    queueCreateInfos[i].pQueuePriorities = &queuePriority;
  }

  VkPhysicalDeviceFeatures deviceFeatures = {0};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceCreateInfo = {0};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos =
      (VkDeviceQueueCreateInfo *)queueCreateInfos;
  deviceCreateInfo.queueCreateInfoCount = uniqueCount;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  // Device extensions
  Grr_u32 propertyCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propertyCount,
                                       NULL);
  VkExtensionProperties *properties = (VkExtensionProperties *)malloc(
      sizeof(VkExtensionProperties) * propertyCount);
  if (properties == NULL) {
    GRR_LOG_CRITICAL(
        "Failed to allocate memory to enumerate device extension properties\n");
    exit(EXIT_FAILURE);
  }
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propertyCount,
                                       properties);
  const Grr_string extensionNames[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(GRR_PLATFORM_MACOS)
    "VK_KHR_portability_subset"
#endif
  };

  Grr_u32 extensionCount = 1;
#if defined(GRR_PLATFORM_MACOS)
  extensionCount += 1;
#endif

  GRR_LOG_DEBUG("Device extensions\n");
  for (Grr_u32 i = 0; i < extensionCount; i++) {
    GRR_LOG_DEBUG("\t%s\n", extensionNames[i]);
  }

  Grr_bool found;
  Grr_bool extensionsOk = true;
  for (Grr_u32 j = 0; j < extensionCount && extensionsOk; j++) {
    found = false;
    for (Grr_u32 i = 0; i < propertyCount && !found; i++) {
      if (strcmp(properties[i].extensionName, extensionNames[j]) == 0) {
        found = true;
      }
    }
    if (!found) {
      GRR_LOG_CRITICAL("Failed to find required device extension %s\n",
                       extensionNames[j]);
      extensionsOk = false;
    }
  }
  free(properties);

  if (!extensionsOk)
    return false;

  deviceCreateInfo.ppEnabledExtensionNames =
      (const char *const *)extensionNames;
  deviceCreateInfo.enabledExtensionCount = extensionCount;

  // Create logical device and queues associated with device
  if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) !=
      VK_SUCCESS)
    return false;

  // Grab handles to queues
  vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamilyIndex, 0,
                   &graphicsQueue);
  vkGetDeviceQueue(device, queueFamilyIndices.presentFamilyIndex, 0,
                   &presentQueue);
  if (queueFamilyIndices.computeFamilyIndex != -1)
    vkGetDeviceQueue(device, queueFamilyIndices.computeFamilyIndex, 0,
                     &computeQueue);
  // TODO: grab other queue handles if needed: video decode, video encode,
  // protected memory management, sparse memory management, and transfer

  atexit(_Grr_destroyLogicalDevice);

  return true;
}

void _Grr_destroySurface() {
  GRR_LOG_INFO("Free surface\n");
  PFN_vkDestroySurfaceKHR fpDestroySurfaceKHR =
      (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance,
                                                     "vkDestroySurfaceKHR");
  if (fpDestroySurfaceKHR)
    fpDestroySurfaceKHR(instance, surface, NULL);
}

Grr_bool _Grr_createSurface() {
#if defined(GRR_PLATFORM_MACOS)
  VkMetalSurfaceCreateInfoEXT macOSCreateInfo = {0};
  macOSCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
  macOSCreateInfo.pLayer = Grr_metalLayer();
  PFN_vkCreateMetalSurfaceEXT fpCreateMetalSurfaceEXT =
      (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(
          instance, "vkCreateMetalSurfaceEXT");
  if (fpCreateMetalSurfaceEXT) {
    if (fpCreateMetalSurfaceEXT(instance, &macOSCreateInfo, NULL, &surface) !=
        VK_SUCCESS)
      return false;
  } else {
    GRR_LOG_CRITICAL("vkCreateMetalSurfaceEXT not available\n");
    return false;
  }

  atexit(_Grr_destroySurface);

  return true;
#endif
}

void _Grr_findQueueFamilies() {
  Grr_u32 queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           NULL);
  VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *)malloc(
      sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  if (queueFamilies != NULL) {
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilies);

    queueFamilyIndices.graphicsFamilyIndex = -1;
    queueFamilyIndices.computeFamilyIndex = -1;
    queueFamilyIndices.transferFamilyIndex = -1;
    queueFamilyIndices.sparseBndingFamilyIndex = -1;
    queueFamilyIndices.presentFamilyIndex = -1;

    VkBool32 presentSupport;
    for (Grr_u32 i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        queueFamilyIndices.graphicsFamilyIndex = i;
      if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        queueFamilyIndices.computeFamilyIndex = i;
      if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        queueFamilyIndices.transferFamilyIndex = i;
      if (queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        queueFamilyIndices.sparseBndingFamilyIndex = i;

      presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                           &presentSupport);
      if (presentSupport) {
        queueFamilyIndices.presentFamilyIndex = i;
      }
    }
    free(queueFamilies);
  } else {
    GRR_LOG_CRITICAL(
        "Failed to allocate memory to get queue family properties\n");
    exit(EXIT_FAILURE);
  }
}

VkSurfaceFormatKHR _Grr_selectFormat(VkSurfaceFormatKHR *formats,
                                     Grr_u32 formatCount) {
  for (Grr_u32 i = 0; i < formatCount; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return formats[i];
    }
  }

  return formats[0];
}

VkPresentModeKHR _Grr_selectPresentMode(VkPresentModeKHR *presentModes,
                                        Grr_u32 presentModeCount) {
  for (Grr_u32 i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return presentModes[i];
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to exist
}

VkExtent2D _Grr_selectExtenct(VkSurfaceCapabilitiesKHR capabilities) {
  return capabilities.currentExtent; // TODO
}

void _Grr_swapchainResizeHandler(void *sender, const GrrEventData data) {
  if (data.window.w > 0 && data.window.h > 0)
    recreateSwapChain = true;
}

void _Grr_destroySwapchain() {
  GRR_LOG_INFO("Free swapchain\n");
  vkDestroySwapchainKHR(device, swapchain, NULL);
  if (swapchainImages != NULL)
    free(swapchainImages);
}

Grr_bool _Grr_createSwapchain(Grr_bool recreateFlag) {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  VkPresentModeKHR *presentModes;

  // Surface capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &capabilities);

  // Formats
  Grr_u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       NULL);
  if (formatCount == 0) {
    GRR_LOG_CRITICAL("No surface formats available\n");
    return false;
  }

  formats =
      (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       formats);
  // Work with formats
  if (formats != NULL) {
    selectedFormat = _Grr_selectFormat(formats, formatCount);
    free(formats);
  } else {
    GRR_LOG_CRITICAL(
        "Failed to allocate memory to get device sruface formats\n");
    return false;
  }

  // Present modes
  Grr_u32 presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, NULL);
  if (presentModeCount == 0) {
    GRR_LOG_CRITICAL("No surface present modes available\n");
    return false;
  }

  presentModes =
      (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, presentModes);
  // Work with present modes
  if (presentModes != NULL) {
    selectedPresentMode =
        _Grr_selectPresentMode(presentModes, presentModeCount);
    free(presentModes);
  } else {
    GRR_LOG_CRITICAL(
        "Failed to allocate memory to get device surface present modes\n");
    return false;
  }

  // Extent
  selectedExtent = _Grr_selectExtenct(capabilities);

  // Swap chain
  Grr_bool hasMaxLimit = capabilities.maxImageCount !=
                         0; // Special value 0 means no max image count limit
  imageCount =
      capabilities.minImageCount + 1; // Attempt to pick 1 on top of min
  if (hasMaxLimit) {
    // Make sure we are not picking more than max possible
    if (capabilities.maxImageCount < imageCount)
      imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = selectedFormat.format;
  createInfo.imageColorSpace = selectedFormat.colorSpace;
  createInfo.imageExtent = selectedExtent;
  createInfo.imageArrayLayers =
      1; // For non-stereoscopic-3D applications, this value is 1
  createInfo.imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Other usage like post processing
  // is possible
  Grr_u32 indices[] = {queueFamilyIndices.graphicsFamilyIndex,
                       queueFamilyIndices.presentFamilyIndex};

  if (queueFamilyIndices.graphicsFamilyIndex !=
      queueFamilyIndices.presentFamilyIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = indices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;  // Optional
    createInfo.pQueueFamilyIndices = NULL; // Optional
  }
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = selectedPresentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO

  if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain) != VK_SUCCESS)
    return false;

  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
  swapchainImages = (VkImage *)malloc(sizeof(VkImage) * imageCount);
  if (swapchainImages == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for swap chain images\n");
    return false;
  }
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);

  if (!recreateFlag)
    atexit(_Grr_destroySwapchain);

  return true;
}

VkImageView _Grr_createImageView(VkImage image, VkFormat format,
                                 VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo viewInfo = {0};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to create texture image view\n");
    exit(EXIT_FAILURE);
  }

  return imageView;
}

void _Grr_destroyImageViews() {
  GRR_LOG_INFO("Free swapchain image views\n");
  Grr_u32 viewCount = imageCount;
  for (Grr_u32 i = 0; i < viewCount; i++)
    vkDestroyImageView(device, imageViews[i], NULL);
  free(imageViews);
}

Grr_bool _Grr_createImageViews(Grr_bool recreate) {
  imageViews = (VkImageView *)malloc(sizeof(VkImageView) * imageCount);
  if (imageViews == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for image views\n");
    return false;
  }

  for (Grr_u32 i = 0; i < imageCount; i++) {
    imageViews[i] = _Grr_createImageView(
        swapchainImages[i], selectedFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

    // VkImageViewCreateInfo createInfo = (VkImageViewCreateInfo){0};
    // createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // createInfo.image = swapchainImages[i];
    // createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // createInfo.format = selectedFormat.format;
    // createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    // createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    // createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    // createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // createInfo.subresourceRange.baseMipLevel = 0;
    // createInfo.subresourceRange.levelCount = 1;
    // createInfo.subresourceRange.baseArrayLayer = 0;
    // createInfo.subresourceRange.layerCount = 1;

    // if (vkCreateImageView(device, &createInfo, NULL, &imageViews[i]) !=
    //     VK_SUCCESS) {
    //   return false;
    // }
  }

  if (!recreate)
    atexit(_Grr_destroyImageViews);

  return true;
}

Grr_bool _Grr_createShaderModule(const Grr_byte *bytes, size_t nBytes,
                                 VkShaderModule *shaderModule) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = nBytes;
  createInfo.pCode = (const Grr_u32 *)bytes;
  if (vkCreateShaderModule(device, &createInfo, NULL, shaderModule) !=
      VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to create shader module\n");
    return false;
  }

  return true;
}

void _Grr_destroyGraphicsPipeline() {
  GRR_LOG_INFO("Free graphics pipeline\n");
  vkDestroyShaderModule(device, fragShaderModule, NULL);
  vkDestroyShaderModule(device, vertShaderModule, NULL);
  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
}

void _Grr_destroyRenderPass() {
  GRR_LOG_INFO("Free render pass\n");
  vkDestroyRenderPass(device, renderPass, NULL);
}

void _Grr_destroyDescriptorSetLayout() {
  GRR_LOG_INFO("Free descriptor set layout\n");
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
}

Grr_bool _Grr_createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = NULL; // Optional

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = NULL;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding,
                                             samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
  layoutInfo.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyDescriptorSetLayout);

  return true;
}

Grr_bool _Grr_createGraphicsPipeline() {
  size_t nBytes;

  // Vertex shader
  Grr_byte *vertShaderBytes =
      Grr_readBytesFromFile("src/shaders/vert.spv", &nBytes);
  if (_Grr_createShaderModule(vertShaderBytes, nBytes, &vertShaderModule) ==
      false) {
    return false;
  }
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  // Fragment shader
  Grr_byte *fragShaderBytes =
      Grr_readBytesFromFile("src/shaders/frag.spv", &nBytes);
  if (_Grr_createShaderModule(fragShaderBytes, nBytes, &fragShaderModule) ==
      false) {
    return false;
  }
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  // Dynamic state
  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount =
      sizeof(dynamicStates) / sizeof(VkDynamicState);
  dynamicState.pDynamicStates = &dynamicStates[0];

  // Vertex input data binding
  Grr_u32 bindingDescriptionCount;
  VkVertexInputBindingDescription *bindingDescriptions =
      Grr_getBindingDescriptions(&bindingDescriptionCount);

  // Vertex input attribute descriptions
  Grr_u32 attributeDescriptionCount;
  VkVertexInputAttributeDescription *attributeDescriptions =
      Grr_getAtributeDescriptions(&attributeDescriptionCount);

  // Vertex input create info
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptionCount;
  vertexInputInfo.pVertexBindingDescriptions =
      bindingDescriptions; // TODO: free where ?
  vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptionCount;
  vertexInputInfo.pVertexAttributeDescriptions =
      attributeDescriptions; // TODO: free where ?

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport and Scissors
  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (Grr_f32)selectedExtent.width;
  viewport.height = (Grr_f32)selectedExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = selectedExtent;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // TODO: change?
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = NULL;               // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  // Color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;         // Optional
  pipelineLayoutInfo.pSetLayouts = NULL;         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                             &pipelineLayout) != VK_SUCCESS) {
    return false;
  }

  // Depth stencil
  VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  // depthStencil.front = {0}; // Optional
  // depthStencil.back = {0};  // Optional

  // Pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional
  pipelineInfo.pDepthStencilState = &depthStencil;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
                                &graphicsPipeline) != VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyGraphicsPipeline);
  return true;
}

VkFormat _Grr_findSupportedFormat(const VkFormat *candidates,
                                  Grr_u32 countCandidates, VkImageTiling tiling,
                                  VkFormatFeatureFlags features) {
  VkFormat format;
  VkFormatProperties props;
  for (Grr_u32 i = 0; i < countCandidates; i++) {
    format = candidates[i];
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  GRR_LOG_CRITICAL("Failed to find supported format\n");
  exit(EXIT_FAILURE);
}

VkFormat _Grr_findDepthFormat() {
  VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                           VK_FORMAT_D24_UNORM_S8_UINT};
  return _Grr_findSupportedFormat(
      &candidates[0], sizeof(candidates) / sizeof(candidates[0]),
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Grr_bool _Grr_createRenderPass() {
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = selectedFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {0};
  depthAttachment.format = _Grr_findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {0};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
  renderPassInfo.pAttachments = &attachments[0];
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;

  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) !=
      VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyRenderPass);
  return true;
}

void _Grr_destroyFramebuffers() {
  GRR_LOG_INFO("Free framebuffers\n");
  for (Grr_u32 i = 0; i < imageCount; i++) {
    vkDestroyFramebuffer(device, swapchainFramebuffers[i], NULL);
  }
  free(swapchainFramebuffers);
}

Grr_bool _Grr_createFramebuffers(Grr_bool recreate) {
  swapchainFramebuffers =
      (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * imageCount);
  if (swapchainFramebuffers == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for swapchain framebuffers\n");
    return false;
  }
  for (size_t i = 0; i < imageCount; i++) {
    VkImageView attachments[] = {imageViews[i], depthImageView};

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount =
        sizeof(attachments) / sizeof(attachments[0]);
    framebufferInfo.pAttachments = &attachments[0];
    framebufferInfo.width = selectedExtent.width;
    framebufferInfo.height = selectedExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, NULL,
                            &swapchainFramebuffers[i]) != VK_SUCCESS) {
      return false;
    }
  }

  if (!recreate)
    atexit(_Grr_destroyFramebuffers);
  return true;
}

void _Grr_destroyCommandPool() {
  GRR_LOG_INFO("Free command pool\n");
  vkDestroyCommandPool(device, commandPool, NULL);
}

Grr_bool _Grr_createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer
  // allocated from a pool to be individually reset to the initial state; either
  // by calling vkResetCommandBuffer, or via the implicit reset when calling
  // vkBeginCommandBuffer
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;

  if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) !=
      VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyCommandPool);

  return true;
}

Grr_u32 _Grr_findMemoryType(Grr_u32 typeFilter,
                            VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (Grr_u32 i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  GRR_LOG_CRITICAL("Failed to find suitable memory type\n");
  exit(EXIT_FAILURE);
}

Grr_bool _Grr_createImage(Grr_u32 width, Grr_u32 height, VkFormat format,
                          VkImageTiling tiling, VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkImage *image,
                          VkDeviceMemory *imageMemory) {
  VkImageCreateInfo imageInfo = {0};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional
  if (vkCreateImage(device, &imageInfo, NULL, image) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to create image\n");
    return false;
  }
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, *image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = _Grr_findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to allocate image memory\n");
    return false;
  }

  vkBindImageMemory(device, *image, *imageMemory, 0);
  return true;
}

bool _Grr_hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void _Grr_destroyDepthResources() {
  vkDestroyImageView(device, depthImageView, NULL);
  vkDestroyImage(device, depthImage, NULL);
  vkFreeMemory(device, depthImageMemory, NULL);
}

Grr_bool _Grr_createDepthResources() {
  VkFormat depthFormat = _Grr_findDepthFormat();
  _Grr_createImage(
      selectedExtent.width, selectedExtent.height, depthFormat,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory);
  depthImageView =
      _Grr_createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  atexit(_Grr_destroyDepthResources);
  return true;
}

void _Grr_freeCommandBuffers() { free(commandBuffers); }

Grr_bool _Grr_createCommandBuffers() {

  commandBuffers =
      (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
  if (commandBuffers == NULL) {
    GRR_LOG_ERROR("Failed to allocate memory for command buffers\n");
    return false;
  }

  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) !=
      VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_freeCommandBuffers);

  return true;
}

Grr_bool _Grr_recordCommandBuffer(VkCommandBuffer commandBuffer,
                                  Grr_u32 imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;               // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    return false;
  }

  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];

  renderPassInfo.renderArea.offset.x = 0;
  renderPassInfo.renderArea.offset.y = 0;
  renderPassInfo.renderArea.extent = selectedExtent;

  VkClearValue clearValues[2];
  clearValues[0].color.float32[0] = 0.0f;
  clearValues[0].color.float32[1] = 0.0f;
  clearValues[0].color.float32[2] = 0.0f;
  clearValues[0].color.float32[3] = 1.0f;
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;
  renderPassInfo.pClearValues = &clearValues[0];
  renderPassInfo.clearValueCount = 2;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0,
                         sizeof(vertexBuffers[0]) / sizeof(vertexBuffers),
                         &vertexBuffers[0], &offsets[0]);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (Grr_f32)selectedExtent.width;
  viewport.height = (Grr_f32)selectedExtent.height;
  // Min and max depth should be the same ones used for calculations by the
  // perspective projection matrix
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = selectedExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSets[currentFrame],
                          0, NULL);
  vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    return false;
  }

  return true;
}

void _Grr_destroySyncObjects() {
  GRR_LOG_INFO("Free synchronization objects\n");
  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
    vkDestroyFence(device, inFlightFences[i], NULL);
  }
  if (imageAvailableSemaphores != NULL)
    free(imageAvailableSemaphores);
  if (renderFinishedSemaphores != NULL)
    free(renderFinishedSemaphores);
  if (inFlightFences != NULL)
    free(inFlightFences);
}

Grr_bool _Grr_createSyncObjects() {
  imageAvailableSemaphores =
      (VkSemaphore *)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores =
      (VkSemaphore *)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  inFlightFences = (VkFence *)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, NULL,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) !=
            VK_SUCCESS) {
      return false;
    }
  }

  atexit(_Grr_destroySyncObjects);
  return true;
}

Grr_bool _Grr_recreateSwapchain() {
  vkDeviceWaitIdle(device);

  _Grr_destroyDepthResources();
  _Grr_destroyFramebuffers();
  _Grr_destroyImageViews();
  _Grr_destroySwapchain();

  GRR_LOG_INFO("Recreate swapchain\n");
  if (!_Grr_createSwapchain(true) || !_Grr_createImageViews(true) ||
      !_Grr_createDepthResources() || !_Grr_createFramebuffers(true)) {
    return false;
  }

  return true;
}

void _Grr_updateUniformBuffer(Grr_u32 currentFrame) {
  GrrUniformBufferObject ubo;
  Grr_identityMatrix(&ubo.model);
  Grr_identityMatrix(&ubo.view);
  Grr_identityMatrix(&ubo.projection);
  memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void Grr_drawFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  Grr_u32 imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    if (false == _Grr_recreateSwapchain()) {
      GRR_LOG_CRITICAL("Failed to recreate swapchain\n");
      exit(EXIT_FAILURE);
    }
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    GRR_LOG_CRITICAL("Failed to acquire swapchain image\n");
    exit(EXIT_FAILURE);
  }

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  _Grr_recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  _Grr_updateUniformBuffer(currentFrame);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &waitSemaphores[0];
  submitInfo.pWaitDstStageMask = &waitStages[0];
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSemaphores[0];

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to submit draw command buffer!");
    exit(EXIT_FAILURE);
  }

  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapChains[0];
  presentInfo.pImageIndices = &imageIndex;

  presentInfo.pResults = NULL; // Optional

  result = vkQueuePresentKHR(presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      recreateSwapChain) {
    recreateSwapChain = false;
    if (false == _Grr_recreateSwapchain()) {
      GRR_LOG_CRITICAL("Failed to recreate swapchain\n");
      exit(EXIT_FAILURE);
    }
  } else if (result != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to present swapchain image\n");
    exit(EXIT_FAILURE);
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void _Grr_destroyVertexBuffer() {
  GRR_LOG_INFO("Free vertex buffer\n");
  vkDestroyBuffer(device, vertexBuffer, NULL);
  vkFreeMemory(device, vertexBufferMemory, NULL);
}

Grr_bool _Grr_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkBuffer *buffer,
                           VkDeviceMemory *bufferMemory) {
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to create buffer\n");
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      _Grr_findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to allocate buffer memory\n");
    return false;
  }

  if (vkBindBufferMemory(device, *buffer, *bufferMemory, 0) != VK_SUCCESS) {
    GRR_LOG_CRITICAL("Failed to bind buffer memory\n");
    return false;
  }

  return true;
}

VkCommandBuffer _Grr_beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void _Grr_endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void _Grr_transitionImageLayout(VkImage image, VkFormat format,
                                VkImageLayout oldLayout,
                                VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = _Grr_beginSingleTimeCommands();

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0; // TODO
  barrier.dstAccessMask = 0; // TODO

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    GRR_LOG_CRITICAL("Unsupported layout transition\n");
    exit(EXIT_FAILURE);
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL,
                       0, NULL, 1, &barrier);

  _Grr_endSingleTimeCommands(commandBuffer);
}

void _Grr_copyBufferToImage(VkBuffer buffer, VkImage image, Grr_u32 width,
                            Grr_u32 height) {
  VkCommandBuffer commandBuffer = _Grr_beginSingleTimeCommands();

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset.x = 0;
  region.imageOffset.y = 0;
  region.imageOffset.z = 0;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  _Grr_endSingleTimeCommands(commandBuffer);
}

void _Grr_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                     VkDeviceSize size) {
  VkCommandBuffer commandBuffer = _Grr_beginSingleTimeCommands();

  VkBufferCopy copyRegion = {0};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  _Grr_endSingleTimeCommands(commandBuffer);
}

void _Grr_destroyTextureImage() {
  GRR_LOG_INFO("Free texture image\n");
  vkDestroyImage(device, textureImage, NULL);
  vkFreeMemory(device, textureImageMemory, NULL);
}

Grr_bool _Grr_createTextureImage() {
  Grr_u32 nBytes;
  Grr_u32 width, height;
  Grr_byte *pixelData;
  pixelData = Grr_loadPNG("./assets/coat_of_arms_of_morocco.png", &nBytes,
                          &width, &height);
  if (pixelData == NULL) {
    GRR_LOG_CRITICAL("Failed to load PNG texture image (%s)\n",
                     "./assets/coat_of_arms_of_morocco.png");
    return false;
  }
  VkDeviceSize imageSize = width * height * 4;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  _Grr_createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixelData, imageSize);
  vkUnmapMemory(device, stagingBufferMemory);
  free(pixelData);

  if (!_Grr_createImage(
          width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage,
          &textureImageMemory)) {
    GRR_LOG_CRITICAL("Failed to create texture image\n");
    return false;
  }

  _Grr_transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  _Grr_copyBufferToImage(stagingBuffer, textureImage, width, height);
  _Grr_transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);

  atexit(_Grr_destroyTextureImage);
  return true;
}

void _Grr_destroyTextureImageView() {
  vkDestroyImageView(device, textureImageView, NULL);
}

void _Grr_createTextureImageView() {
  textureImageView = _Grr_createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                          VK_IMAGE_ASPECT_COLOR_BIT);

  atexit(_Grr_destroyTextureImageView);
}

void _Grr_destroyTextureSampler() {
  vkDestroySampler(device, textureSampler, NULL);
}

Grr_bool _Grr_createTextureSampler() {
  VkSamplerCreateInfo samplerInfo = {0};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  VkPhysicalDeviceProperties properties = {0};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(device, &samplerInfo, NULL, &textureSampler) !=
      VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyTextureSampler);
  return true;
}

void _Grr_modelDebug(GrrModel *model) {
  printf("Vertex count %u\n", model->vertexCount);
  for (Grr_u32 i = 0; i < model->vertexCount * 3; i += 3) {
    printf("%.1f %.1f %.1f\n", model->positions[i], model->positions[i + 1],
           model->positions[i + 2]);
  }

  printf("Index count %u\n", model->indexCount);
  for (Grr_u32 i = 0; i < model->indexCount; i++) {
    printf("%u ", model->indices[i]);
  }
  printf("\n");
}

Grr_bool _Grr_createVertexBuffer() {
  // TODO: temp
  // model.vertexCount = 4;
  // model.vertices = (GrrVertex *)malloc(sizeof(GrrVertex) *
  // model.vertexCount); model.vertices[0].position[0] = -0.5f;
  // model.vertices[0].position[1] = -0.5f;
  // model.vertices[0].position[2] = 0.5f;
  // model.vertices[0].color[0] = 1.0f;
  // model.vertices[0].color[1] = 0.0f;
  // model.vertices[0].color[2] = 0.0f;
  // model.vertices[0].color[3] = 0.0f;
  // model.vertices[0].textureCoordinates[0] = 0.0f;
  // model.vertices[0].textureCoordinates[1] = 0.0f;

  // model.vertices[1].position[0] = 0.5f;
  // model.vertices[1].position[1] = -0.5f;
  // model.vertices[1].position[2] = 0.5f;
  // model.vertices[1].color[0] = 0.0f;
  // model.vertices[1].color[1] = 1.0f;
  // model.vertices[1].color[2] = 0.0f;
  // model.vertices[1].color[3] = 0.0f;
  // model.vertices[1].textureCoordinates[0] = 1.0f;
  // model.vertices[1].textureCoordinates[1] = 0.0f;

  // model.vertices[2].position[0] = 0.5f;
  // model.vertices[2].position[1] = 0.5f;
  // model.vertices[2].position[2] = 0.5f;
  // model.vertices[2].color[0] = 0.0f;
  // model.vertices[2].color[1] = 0.0f;
  // model.vertices[2].color[2] = 1.0f;
  // model.vertices[2].color[3] = 0.0f;
  // model.vertices[2].textureCoordinates[0] = 1.0f;
  // model.vertices[2].textureCoordinates[1] = 1.0f;

  // model.vertices[3].position[0] = -0.5f;
  // model.vertices[3].position[1] = 0.5f;
  // model.vertices[3].position[2] = 0.5f;
  // model.vertices[3].color[0] = 1.0f;
  // model.vertices[3].color[1] = 1.0f;
  // model.vertices[3].color[2] = 1.0f;
  // model.vertices[3].color[3] = 0.0f;
  // model.vertices[3].textureCoordinates[0] = 0.0f;
  // model.vertices[3].textureCoordinates[1] = 1.0f;

  // model.indexCount = 6;
  // model.indices = (Grr_u32 *)malloc(sizeof(Grr_u32) * model.indexCount);
  // model.indices[0] = 0;
  // model.indices[1] = 1;
  // model.indices[2] = 2;
  // model.indices[3] = 2;
  // model.indices[4] = 3;
  // model.indices[5] = 0;

  // Test loading models here
  GrrAssetglTF *glTF = Grr_glTFLoad("../glTF-Sample-Models/2.0/DamagedHelmet/"
                                    "glTF/DamagedHelmet.gltf");
  if (NULL == glTF) {
    GRR_LOG_CRITICAL("Failed to load glTF asset\n");
    exit(EXIT_FAILURE);
  }
  // Grr_writeJSONToFile(json, "output.json");
  Grr_modelFromAsset(&model, glTF, 0, 0);
  _Grr_modelDebug(&model);
  VkDeviceSize positionBufferSize =
      (model.positions ? sizeof(Grr_f32) * 3 * model.vertexCount
                       : 0); // Positions
  // VkDeviceSize colorBufferSize =
  //     (model.colors ? sizeof(Grr_f32) * 3 * model.vertexCount : 0); // Colors
  // VkDeviceSize textureCoordinateBufferSize =
  //     (model.textureCoordinates ? sizeof(Grr_f32) * 2 * model.vertexCount
  //                               : 0); // Texture coordinates
  VkDeviceSize bufferSize =
      positionBufferSize; // + colorBufferSize + textureCoordinateBufferSize;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  if (_Grr_createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &stagingBuffer, &stagingBufferMemory) == false) {
    GRR_LOG_CRITICAL("Failed to create staging buffer\n");
    return false;
  }

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  if (NULL != model.positions)
    memcpy(data, model.positions, (size_t)positionBufferSize); // Positions
  // if (NULL != model.colors)
  //   memcpy(data + positionBufferSize, model.colors,
  //          (size_t)colorBufferSize); // Colors
  // if (NULL != model.textureCoordinates)
  //   memcpy(data + positionBufferSize + colorBufferSize,
  //          model.textureCoordinates,
  //          (size_t)textureCoordinateBufferSize); // Texture coordinates
  vkUnmapMemory(device, stagingBufferMemory);

  if (_Grr_createBuffer(bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer,
                        &vertexBufferMemory) == false) {
    GRR_LOG_CRITICAL("Failed to create vertex buffer\n");
    return false;
  }

  _Grr_copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);

  atexit(_Grr_destroyVertexBuffer);

  return true;
}

void _Grr_destroyIndexBuffer() {
  GRR_LOG_INFO("Free index buffer\n");
  vkDestroyBuffer(device, indexBuffer, NULL);
  vkFreeMemory(device, indexBufferMemory, NULL);
}

void _Grr_destroyUniformBuffers() {
  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], NULL);
    vkFreeMemory(device, uniformBuffersMemory[i], NULL);
  }

  free(uniformBuffers);
  free(uniformBuffersMapped);
  free(uniformBuffersMemory);

  // vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

Grr_bool _Grr_createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(GrrUniformBufferObject);

  uniformBuffers = (VkBuffer *)malloc(sizeof(VkBuffer) * MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory =
      (VkDeviceMemory *)malloc(sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped = (void **)malloc(sizeof(void *) * MAX_FRAMES_IN_FLIGHT);

  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (_Grr_createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &uniformBuffers[i],
                          &uniformBuffersMemory[i]) == false) {
      return false;
    }

    if (vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                    &uniformBuffersMapped[i]) != VK_SUCCESS) {
      return false;
    }
  }

  atexit(_Grr_destroyUniformBuffers);
  return true;
}

void _Grr_destroyDescriptorPool() {
  GRR_LOG_INFO("Free descriptor pool\n");
  vkDestroyDescriptorPool(device, descriptorPool, NULL);
}

Grr_bool _Grr_createDescriptorPool() {
  VkDescriptorPoolSize poolSizes[2] = {0};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

  if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) !=
      VK_SUCCESS) {
    return false;
  }

  atexit(_Grr_destroyDescriptorPool);

  return true;
}

void _Grr_destroyDescriptorSets() {
  GRR_LOG_INFO("Free descriptor sets\n");
  free(descriptorSets);
  free(layouts);
}

Grr_bool _Grr_createDescriptorSets() {
  layouts = malloc(sizeof(VkDescriptorSetLayout) * MAX_FRAMES_IN_FLIGHT);
  if (layouts == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for descriptor set layouts\n");
    return false;
  }
  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    layouts[i] = descriptorSetLayout;

  VkDescriptorSetAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  allocInfo.pSetLayouts = layouts;

  descriptorSets = malloc(sizeof(VkDescriptorSet) * MAX_FRAMES_IN_FLIGHT);
  if (descriptorSets == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for descriptor sets\n");
    return false;
  }

  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets) !=
      VK_SUCCESS) {
    return false;
  }

  for (Grr_u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GrrUniformBufferObject);

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    VkWriteDescriptorSet descriptorWrites[2] = {0};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(
        device, sizeof(descriptorWrites) / sizeof(descriptorWrites[0]),
        &descriptorWrites[0], 0, NULL);
  }

  atexit(_Grr_destroyDescriptorSets);
  return true;
}

Grr_bool _Grr_createIndexBuffer() {
  VkDeviceSize bufferSize = sizeof(Grr_u32) * model.indexCount;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  if (_Grr_createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &stagingBuffer, &stagingBufferMemory) == false) {
    GRR_LOG_CRITICAL("Failed to create staging buffer");
    return false;
  }

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, model.indices, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  if (_Grr_createBuffer(bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer,
                        &indexBufferMemory) == false) {
    GRR_LOG_CRITICAL("Failed to create index buffer");
    return false;
  }

  _Grr_copyBuffer(stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);

  atexit(_Grr_destroyIndexBuffer);
  return true;
}

void _Grr_deviceWait() { vkDeviceWaitIdle(device); }

void Grr_initializeVulkan() {
  GRR_LOG_INFO("Initialize vulkan\n");

  // Instance and debugger callback
  if (false == _Grr_createVulkanInstance()) {
    GRR_LOG_CRITICAL("Failed to create vulkan instance\n");
    exit(EXIT_FAILURE);
  }

  // Window surface
  if (false == _Grr_createSurface()) {
    GRR_LOG_CRITICAL("Failed to create vulkan surface\n");
    exit(EXIT_FAILURE);
  }

  // Physical device
  if (false == _Grr_selectPhysicalDevice()) {
    GRR_LOG_CRITICAL("Failed to select physical device\n");
    exit(EXIT_FAILURE);
  }

  // Queue families supported by physical device
  _Grr_findQueueFamilies();

  // Logical device to interface with physical device
  if (false == _Grr_createLogicalDevice()) {
    GRR_LOG_CRITICAL("Failed to create logical device\n");
    exit(EXIT_FAILURE);
  }

  // Swap chain
  if (false == _Grr_createSwapchain(false)) {
    GRR_LOG_CRITICAL("Failed to create swapchain\n");
    exit(EXIT_FAILURE);
  }
  if (false == Grr_subscribe(GRR_WINDOW_RESIZED, _Grr_swapchainResizeHandler)) {
    GRR_LOG_CRITICAL("Failed to subscribe swapchain resize handler\n");
    exit(EXIT_FAILURE);
  }

  // Image views
  if (false == _Grr_createImageViews(false)) {
    GRR_LOG_CRITICAL("Failed to create image views\n");
    exit(EXIT_FAILURE);
  }

  // Render pass
  if (false == _Grr_createRenderPass()) {
    GRR_LOG_CRITICAL("Failed to create render pass\n");
    exit(EXIT_FAILURE);
  }

  // Descriptor set layout
  if (false == _Grr_createDescriptorSetLayout()) {
    GRR_LOG_CRITICAL("Failed to create descriptor set layout");
    exit(EXIT_FAILURE);
  }

  // Graphics pipeline
  if (false == _Grr_createGraphicsPipeline()) {
    GRR_LOG_CRITICAL("Failed to create graphics pipeline\n");
    exit(EXIT_FAILURE);
  }

  // Command pool
  if (false == _Grr_createCommandPool()) {
    GRR_LOG_CRITICAL("Failed to create command pool\n");
    exit(EXIT_FAILURE);
  }

  // Depth buffer
  if (false == _Grr_createDepthResources()) {
    GRR_LOG_CRITICAL("Failed to create depth resources\n");
    exit(EXIT_FAILURE);
  }

  // Frame buffers (needs depth resources)
  if (false == _Grr_createFramebuffers(false)) {
    GRR_LOG_CRITICAL("Failed to create frame buffers\n");
    exit(EXIT_FAILURE);
  }

  // Texture image
  if (false == _Grr_createTextureImage()) {
    GRR_LOG_CRITICAL("Failed to create texture image\n");
    exit(EXIT_FAILURE);
  }

  // Texture image view
  _Grr_createTextureImageView();

  // Texture sampler
  if (!_Grr_createTextureSampler()) {
    GRR_LOG_CRITICAL("Failed to create texture sampler\n");
    exit(EXIT_FAILURE);
  }

  // Vertex buffers
  if (false == _Grr_createVertexBuffer()) {
    GRR_LOG_CRITICAL("Failed to create vertex buffer\n");
    exit(EXIT_FAILURE);
  }

  // Index buffers
  if (false == _Grr_createIndexBuffer()) {
    GRR_LOG_CRITICAL("Failed to create index buffer\n");
    exit(EXIT_FAILURE);
  }

  // Uniform buffers
  if (false == _Grr_createUniformBuffers()) {
    GRR_LOG_CRITICAL("Failed to create uniform buffers\n");
    exit(EXIT_FAILURE);
  }

  // Descriptor pool
  if (false == _Grr_createDescriptorPool()) {
    GRR_LOG_CRITICAL("Failed to create descriptor pool\n");
    exit(EXIT_FAILURE);
  }

  // Descriptor sets
  if (false == _Grr_createDescriptorSets()) {
    GRR_LOG_CRITICAL("Failed to create descriptor sets\n");
    exit(EXIT_FAILURE);
  }

  // Command pool
  if (false == _Grr_createCommandBuffers()) {
    GRR_LOG_CRITICAL("Failed to create command buffer\n");
    exit(EXIT_FAILURE);
  }

  // Sync objects
  if (false == _Grr_createSyncObjects()) {
    GRR_LOG_CRITICAL("Failed to create sync objects\n");
    exit(EXIT_FAILURE);
  }

  // Should be last to have it execute first at exit
  atexit(_Grr_deviceWait);
}