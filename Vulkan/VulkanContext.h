#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

class VulkanContext
{
public:
    void init(GLFWwindow* window)
    {
        createInstance();
        setupDebugMessenger();
        createSurface(window);
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPool();
    }

    // Membres publics
    vk::raii::Context                context;
    vk::raii::Instance               instance       = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR             surface        = nullptr;
    vk::raii::PhysicalDevice         physicalDevice = nullptr;
    vk::raii::Device                 device         = nullptr;
    uint32_t                         queueIndex     = ~0u;
    vk::raii::Queue                  queue          = nullptr;
    vk::raii::CommandPool            commandPool    = nullptr;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

private:
    void createInstance();            
    void setupDebugMessenger();
    void createSurface(GLFWwindow*);
    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const&);
    void createLogicalDevice();
    void createCommandPool();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT, vk::DebugUtilsMessageTypeFlagsEXT,
        const vk::DebugUtilsMessengerCallbackDataEXT*, void*);

    std::vector<const char*> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName};
};