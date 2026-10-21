#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <memory>

#include "Renderer.h"

class Application
{
public:
    void Init();
    void Run();
    void Destroy();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* window = nullptr;

private:
    void InitWindow();
    void InitVulkan();

    void DestroyVulkan();
    void DestroyWindow();

    // unique_ptr : permet de détruire le Renderer explicitement AVANT la fenêtre
    std::unique_ptr<Renderer> renderer;
};