#include <graphics/vulkan/rendering_device.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <core/engine.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

void RenderingDevice::initialize() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(width, height, "New window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    vulkanContext.initialize(window);

    setupDescriptorSetLayouts();
    createCommandPool();

    updateRenderArea();

    createRenderPipelines();
    createCommandBuffers();
    createSyncObjects();

    //uniformBuffers.reserve(MAX_FRAME_LAG);
    //for (int i = 0; i < MAX_FRAME_LAG; i++) {
    //    uniformBuffers.emplace_back(sizeof(GlobalUniforms));
    //}

    createDescriptorPool();

    createDescriptorSets();
}

void RenderingDevice::setupDescriptorSetLayouts() {
    
}

void RenderingDevice::createRenderPipelines() {
    
}

void RenderingDevice::createCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vulkanContext.graphicsQueueFamilyIndex
    };

    CHECK_VKRESULT(vkCreateCommandPool(vulkanContext.device, &commandPoolCreateInfo, nullptr, &commandPool));
}

void RenderingDevice::createCommandBuffers() {
    
}

void RenderingDevice::buildRenderCommandBuffer() {
    
}

VkResult RenderingDevice::createBuffer(VkBuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceMemory* memory, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, const void* data) {
    // moved into buffer class, should return and store buffer object now (for management)
}

uint32_t RenderingDevice::getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < vulkanContext.memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)) {
            if ((vulkanContext.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
    }

    throw std::runtime_error("ERROR::RenderingDevice:getMemoryType: failed to find suitable memory type!");
}

VkResult RenderingDevice::createCommandBuffer(VkCommandBuffer* buffer, VkCommandBufferLevel level, bool begin) {
    VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = level,
        .commandBufferCount = 1
    };

    VkResult err = vkAllocateCommandBuffers(vulkanContext.device, &allocateInfo, buffer);
    if (err != VK_SUCCESS) {
        return err;
    }

    if (begin) {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        CHECK_VKRESULT(vkBeginCommandBuffer(*buffer, &beginInfo));
    }

    return VK_SUCCESS;
}

void RenderingDevice::commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType) {
    CHECK_VKRESULT(vkEndCommandBuffer(*buffer));

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = buffer
    };

    VkQueue queue;
    switch (queueType) {
        case VK_QUEUE_GRAPHICS_BIT:
            queue = vulkanContext.graphicsQueue;
            break;
        default:
            queue = VK_NULL_HANDLE;
            break;
    }

    CHECK_VKRESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(vulkanContext.device, commandPool, 1, buffer);
}

VkPipelineShaderStageCreateInfo RenderingDevice::loadSPIRVShader(const std::string& filename, VkShaderStageFlagBits stage, std::vector<VkShaderModule>* shaderModules) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = utils::loadShader(filename, vulkanContext.device),
        .pName = "main"
    };

    shaderModules->push_back(shaderStageInfo.module);

    return shaderStageInfo;
}

void RenderingDevice::windowResizeCallback(GLFWwindow* window, int width, int height) {
    auto rd = (RenderingDevice*)glfwGetWindowUserPointer(window);
    rd->windowResized = true;
}

void RenderingDevice::updateGlobalBuffers() {
    
}

void RenderingDevice::updateRenderArea() {
    
}

void RenderingDevice::cleanupRenderArea() {
    
}

void RenderingDevice::render() {
    
}

void RenderingDevice::renderDepth() {
    vkWaitForFences(vulkanContext.device, 1, &depthPassFences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanContext.device, 1, &depthPassFences[frameIndex]);

    vkResetCommandBuffer(depthPrePassCommandBuffers[frameIndex], 0);
    buildPrepassCommandBuffer();

    VkSemaphore waitSemaphores[] = { presentCompleteSemaphores[frameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &depthPrePassCommandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &depthPrePassCompleteSemaphores[frameIndex]
    };

    CHECK_VKRESULT(vkQueueSubmit(vulkanContext.graphicsQueue, 1, &submitInfo, depthPassFences[frameIndex]));
}

void RenderingDevice::renderLighting() {
    
}

RenderingDevice::~RenderingDevice() {
    vkDeviceWaitIdle(vulkanContext.device);

    cleanupRenderArea();

    vkDestroyDescriptorPool(vulkanContext.device, descriptorPool, nullptr);
    for (int i = 0; i < descriptorSetLayouts.size(); i++) {
        vkDestroyDescriptorSetLayout(vulkanContext.device, descriptorSetLayouts[i], nullptr);
    }
    vkDestroyDescriptorSetLayout(vulkanContext.device, depthPassDescriptorSetLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
        vkDestroySemaphore(vulkanContext.device, presentCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanContext.device, renderCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanContext.device, depthPrePassCompleteSemaphores[i], nullptr);
        vkDestroyFence(vulkanContext.device, inFlightFences[i], nullptr);
        vkDestroyFence(vulkanContext.device, depthPassFences[i], nullptr);
    }

    for (auto& shaderModule : shaderModules) {
        vkDestroyShaderModule(vulkanContext.device, shaderModule, nullptr);
    }

    vkDestroyCommandPool(vulkanContext.device, commandPool, nullptr);
    vkDestroyPipeline(vulkanContext.device, renderPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanContext.device, pipelineLayout, nullptr);

    vkDestroyPipeline(vulkanContext.device, depthPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanContext.device, depthPipelineLayout, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

RenderingDevice* RenderingDevice::getSingleton() {
    static RenderingDevice singleton;
    return &singleton;
}

void RenderingDevice::createDescriptorPool() {
    
}

void RenderingDevice::createDescriptorSets() {
    
}

void RenderingDevice::buildPrepassCommandBuffer() {
    
}

void RenderingDevice::updateSwapchain(uint32_t* width, uint32_t* height) {
    vulkanContext.updateSwapchain(window);
    *width = vulkanContext.swapChain.width;
    *height = vulkanContext.swapChain.height;
}

} // namespace vkw

} // namespace sublimation