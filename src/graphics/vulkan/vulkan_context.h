#pragma once

#include <graphics/vulkan/swapchain.h>

#include <volk.h>
#include <glfw/glfw3.h>
#include <vk_mem_alloc.h>

#include <string>
#include <vector>

namespace sublimation {

namespace vkw {

class RenderingDevice;

class VulkanContext {
public:
    ~VulkanContext();

    void initialize(GLFWwindow* window);
    void updateSwapchain(GLFWwindow* window);
    void cleanupSwapchain();

private:
    void createInstance();
    void createPhysicalDevice();
    void createDevice();
    void initializeQueues();

    bool checkDeviceExtensionSupport(VkPhysicalDevice physDevice);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

private:
    friend RenderingDevice;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VmaAllocator allocator;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    uint32_t computeQueueFamilyIndex = UINT32_MAX;
    bool separatePresentQueue = false;

    bool instanceInitialized = false;
    bool deviceInitialized = false;
    bool isValidationLayersEnabled;
    std::vector<std::string> enabledDeviceExtensions;

    Swapchain swapChain;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT CreateUtilsDebugMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyUtilsDebugMessengerEXT = nullptr;
};

} // namespace vkw

} // namespace sublimation