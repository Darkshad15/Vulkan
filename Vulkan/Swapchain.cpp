#include "Swapchain.h"
#include "VulkanHelpers.h"

#include <algorithm>
#include <cassert>
#include <limits>

void Swapchain::create(VulkanContext& ctx, GLFWwindow* window)
{
    imageViews.clear();

    vk::SurfaceCapabilitiesKHR capabilities = ctx.physicalDevice.getSurfaceCapabilitiesKHR(*ctx.surface);
    extent                                  = chooseExtent(capabilities, window);
    uint32_t minImageCount                  = chooseMinImageCount(capabilities);

    auto availableFormats = ctx.physicalDevice.getSurfaceFormatsKHR(*ctx.surface);
    surfaceFormat         = chooseSurfaceFormat(availableFormats);

    auto availablePresentModes = ctx.physicalDevice.getSurfacePresentModesKHR(*ctx.surface);
    vk::PresentModeKHR presentMode = choosePresentMode(availablePresentModes);

    vk::SwapchainCreateInfoKHR createInfo{
        .surface          = *ctx.surface,
        .minImageCount    = minImageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = presentMode,
        .clipped          = true};

    swapChain = vk::raii::SwapchainKHR(ctx.device, createInfo);
    images    = swapChain.getImages();

    imageViews.reserve(images.size());
    for (auto& image : images)
    {
        imageViews.emplace_back(vkh::createImageView(ctx, image, surfaceFormat.format,
                                                     vk::ImageAspectFlagBits::eColor));
    }
}

uint32_t Swapchain::chooseMinImageCount(vk::SurfaceCapabilitiesKHR const& capabilities)
{
    auto minImageCount = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < minImageCount)
    {
        minImageCount = capabilities.maxImageCount;
    }
    return minImageCount;
}

vk::SurfaceFormatKHR Swapchain::chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    assert(!availableFormats.empty());
    const auto it = std::ranges::find_if(availableFormats, [](const auto& f) {
        return f.format == vk::Format::eB8G8R8A8Srgb &&
               f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return it != availableFormats.end() ? *it : availableFormats[0];
}

vk::PresentModeKHR Swapchain::choosePresentMode(std::vector<vk::PresentModeKHR> const& availableModes)
{
    return std::ranges::any_of(availableModes, [](vk::PresentModeKHR m) { return m == vk::PresentModeKHR::eMailbox; })
               ? vk::PresentModeKHR::eMailbox
               : vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseExtent(vk::SurfaceCapabilitiesKHR const& capabilities, GLFWwindow* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}