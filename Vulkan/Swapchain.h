#pragma once
#include "VulkanContext.h"

class Swapchain
{
public:
    void create(VulkanContext& ctx, GLFWwindow* window);   // swapchain + views
    void cleanup() { imageViews.clear(); swapChain = nullptr; }

    vk::raii::SwapchainKHR           swapChain = nullptr;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::SurfaceFormatKHR             surfaceFormat;
    vk::Extent2D                     extent;

private:
    static uint32_t             chooseMinImageCount(vk::SurfaceCapabilitiesKHR const&);
    static vk::SurfaceFormatKHR chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const&);
    static vk::PresentModeKHR   choosePresentMode(std::vector<vk::PresentModeKHR> const&);
    vk::Extent2D                chooseExtent(vk::SurfaceCapabilitiesKHR const&, GLFWwindow*);
};