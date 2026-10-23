# Vulkan – Cube texturé

Rendu d'un cube texturé en rotation avec Vulkan 1.4, C++20 et vulkan_raii.

## Fonctionnalités
- Dynamic rendering
- Depth buffer
- Texture (stb_image)
- Uniform buffer (model / view / proj)
- 2 frames in flight
- Recréation de la swapchain au redimensionnement

## Dépendances

- Vulkan SDK 
- GLFW
- stb_image
- Slang (compilation du shader)

## Ressources nécessaires

- shader/slang.spv (entrées : vertMain, fragMain)
- textures/texture.jpg

## Structure

| Fichier | Rôle |
|---|---|
| VulkanContext | Instance, GPU, device, queue, command pool |
| Vulkan | Buffers, images, transitions de layout |
| Swapchain | Swapchain et image views |
| DepthBuffer | Image de profondeur |
| Texture | Chargement de la texture, vue et sampler |
| Mesh | Vertex et index buffers |
| Descriptors | Descriptor sets et UBO |
| GraphicsPipeline | Pipeline graphique |
| Renderer | Boucle de rendu |
