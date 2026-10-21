#include "Application.h"

#include <iostream>

void Application::Init()
{
    InitWindow();
    InitVulkan();
}

void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderer->drawFrame(window);
    }

    // Attendre que la GPU ait fini avant toute destruction
    renderer->waitIdle();
}

void Application::Destroy()
{
    DestroyVulkan();
    DestroyWindow();
}

void Application::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);   // TRUE pour tester le resize

    window = glfwCreateWindow(800, 600, "Vulkan Triangle", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::InitVulkan()
{
    renderer = std::make_unique<Renderer>();
    renderer->init(window);   // remplace les 18 appels un par un
}

void Application::DestroyVulkan()
{
    // Détruit le Renderer et, dans l'ordre inverse de la création,
    // toutes ses briques (pipeline ... swapchain ... contexte).
    renderer.reset();
}

void Application::DestroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->renderer)
    {
        app->renderer->framebufferResized = true;
    }
}