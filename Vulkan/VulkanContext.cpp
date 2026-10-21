#include "VulkanContext.h"
#include "Debug.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::vector<const char*> getRequiredInstanceExtensions()
    {
        uint32_t count = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
        if (Debug::EnableDebug)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        return extensions;
    }
}

// Instance
void VulkanContext::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName   = "Vulkan triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "Gaming Campus Triangle",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14};

    // Layers
    std::vector<const char*> requiredLayers;
    if (Debug::EnableDebug)
    {
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (const char* layer : requiredLayers)
    {
        bool found = std::ranges::any_of(layerProperties, [layer](auto const& p) {
            return std::strcmp(p.layerName, layer) == 0;
        });
        if (!found)
        {
            throw std::runtime_error("Required layer not supported: " + std::string(layer));
        }
    }

    // Extensions (strcmp == 0 : correction du bug de l'ancien code)
    std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (const char* ext : requiredExtensions)
    {
        bool found = std::ranges::any_of(extensionProperties, [ext](auto const& p) {
            return std::strcmp(p.extensionName, ext) == 0;
        });
        if (!found)
        {
            throw std::runtime_error("Required extension not supported: " + std::string(ext));
        }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames     = requiredLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()};

    instance = vk::raii::Instance(context, createInfo);
}

// ---------------------------------------------------------------------------
// Debug messenger
// ---------------------------------------------------------------------------
void VulkanContext::setupDebugMessenger()
{
    if (!Debug::EnableDebug)
        return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT typeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT createInfo{
        .messageSeverity = severityFlags,
        .messageType     = typeFlags,
        .pfnUserCallback = &debugCallback};

    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanContext::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* data,
    void*)
{
    std::cout << " Validation Layer : Type [" << vk::to_string(type)
              << "] - Message : " << data->pMessage << std::endl;
    return vk::False;
}

// ---------------------------------------------------------------------------
// Surface
// ---------------------------------------------------------------------------
void VulkanContext::createSurface(GLFWwindow* window)
{
    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, rawSurface);
}

// ---------------------------------------------------------------------------
// Physical device
// ---------------------------------------------------------------------------
bool VulkanContext::isDeviceSuitable(vk::raii::PhysicalDevice const& gpu)
{
    // Vulkan 1.4
    bool supportsVulkan = gpu.getProperties().apiVersion >= vk::ApiVersion14;

    // Une queue family graphics + present
    auto queueFamilies = gpu.getQueueFamilyProperties();
    bool supportsGraphicsAndPresent = false;
    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            gpu.getSurfaceSupportKHR(i, *surface))
        {
            supportsGraphicsAndPresent = true;
            break;
        }
    }

    // Extensions de device
    auto availableExtensions = gpu.enumerateDeviceExtensionProperties();
    bool supportsAllExtensions = std::ranges::all_of(requiredDeviceExtensions, [&](const char* required) {
        return std::ranges::any_of(availableExtensions, [required](auto const& available) {
            return std::strcmp(available.extensionName, required) == 0;
        });
    });

    // Features
    auto features = gpu.getFeatures2<vk::PhysicalDeviceFeatures2,
                                     vk::PhysicalDeviceVulkan13Features>();
    bool supportsFeatures =
        features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2;

    return supportsVulkan && supportsGraphicsAndPresent && supportsAllExtensions && supportsFeatures;
}

void VulkanContext::pickPhysicalDevice()
{
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(devices, [&](auto const& d) { return isDeviceSuitable(d); });
    if (it == devices.end())
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    physicalDevice = *it;
}

// ---------------------------------------------------------------------------
// Logical device + queue
// ---------------------------------------------------------------------------
void VulkanContext::createLogicalDevice()
{
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(i, *surface))
        {
            queueIndex = i;
            break;
        }
    }
    if (queueIndex == ~0u)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features>
        featureChain = {
            {.features = {.samplerAnisotropy = true}},              // anisotropie du sampler
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true}};

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()};

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    queue  = vk::raii::Queue(device, queueIndex, 0);
}

// ---------------------------------------------------------------------------
// Command pool
// ---------------------------------------------------------------------------
void VulkanContext::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueIndex};
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

// ---------------------------------------------------------------------------
// Utilitaire mémoire
// ---------------------------------------------------------------------------
uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1u << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}