#pragma once

#include <glfw/glfw3.h>
#include <volk.h>
#include <glm/glm.hpp>
#include <memory>

#include <graphics/vulkan/vulkan_context.h>
#include <graphics/vulkan/command_buffer.h>
#include <graphics/vulkan/pipeline.h>

namespace sublimation {

namespace vkw {

class Descriptor;
class Buffer;
class Texture;
enum TextureType : int;
struct TextureInfo;
struct PipelineInfo;

class RenderingDevice {
protected:
    RenderingDevice() {}

public:
    ~RenderingDevice();
    static RenderingDevice* getSingleton();

    void initialize();
    void render();

    GLFWwindow* getWindow() const { return window; }
    glm::uvec2 getWindowSize() const { return { width, height }; }

    const VkDevice& getDevice() const { return vulkanContext.device; }
    const VkPhysicalDevice& getPhysicalDevice() const { return vulkanContext.physicalDevice; }
    VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return vulkanContext.deviceProperties; }
    VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return vulkanContext.deviceFeatures; }

    const VmaAllocator& getAllocator() const { return vulkanContext.allocator; }

    const VkQueue& getGraphicsQueue() const { return vulkanContext.graphicsQueue; }
    const VkQueue& getComputeQueue() const { return vulkanContext.computeQueue; }
    const Swapchain& getSwapChain() const { return vulkanContext.swapChain; }

    uint32_t getGraphicsQueueFamily() const { return vulkanContext.graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamily() const { return vulkanContext.presentQueueFamilyIndex; }
    uint32_t getComputeQueueFamily() const { return vulkanContext.computeQueueFamilyIndex; }

    const VkDescriptorPool& getDescriptorPool() const { return descriptorPool; }
    const VkDescriptorSetLayout& getDescriptorSetLayout(int index) const { return descriptorSetLayouts[index]; }

    const VkCommandPool& getCommandPool() const { return commandPool; }
    VkSampleCountFlagBits getMSAASamples() const { return multisampling; }

    VkCommandBuffer getCommandBuffer(int frameIdx);
    VkCommandBuffer getCommandBufferOneTime(int frameIdx, bool begin = true);
    void commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType);
    uint32_t getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkPipeline createPipeline(const PipelineInfo& pipelineInfo, const std::vector<Descriptor>& descriptors);

    std::weak_ptr<Buffer> createBuffer(VkBufferUsageFlags usageFlags, VmaMemoryUsage properties, VkDeviceSize size, const void* data = nullptr);
    std::weak_ptr<Texture> createTexture(TextureType type, const glm::ivec2& extent, TextureInfo texInfo);
    std::weak_ptr<Texture> loadTextureFromFile(const std::string& filename, VkFilter filter, VkSamplerAddressMode addressMode, bool aniso, bool mipmap);

    Shader createShaderFromSPIRV(const ShaderStageInfo& shaderInfo);

    void deviceWaitIdle() const { vkDeviceWaitIdle(vulkanContext.device); }
    void updateSwapchain(uint32_t* width, uint32_t* height);

private:

    static void windowResizeCallback(GLFWwindow* window, int width, int height);

private:
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t maxFrameLag = 2;
    bool windowResized = false;

    GLFWwindow* window;
    VulkanContext vulkanContext;
    CommandBufferManager commandBufferManager;

    //< Main render pass (obsolete)
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts; // scene buffers + material images
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline renderPipeline;

    ///< Depth pre-pass (obsolete)
    VkDescriptorSetLayout depthPassDescriptorSetLayout;
    VkPipelineLayout depthPipelineLayout;
    VkRenderPass depthPrePass;
    VkPipeline depthPipeline;

    ///< obsolete
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> depthPrePassCommandBuffers;
    uint32_t frameIndex = 0;

    // Resources to manage
    std::vector<VkPipeline> pipelines;
    std::vector<VkRenderPass> renderPasses;
    std::vector<Shader> shaders;
    std::vector<std::shared_ptr<Buffer>> bufferObjects;
    std::vector<std::shared_ptr<Texture>> textureObjects;

    ///< obsolete
    uint32_t currentBuffer = 0;
    std::vector<VkSemaphore> depthPrePassCompleteSemaphores;
    std::vector<VkFence> depthPassFences;
    std::vector<VkSemaphore> presentCompleteSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;
    std::vector<VkFence> inFlightFences;

    ///< obsolete
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> depthPassDescriptorSets;

    VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_8_BIT;
};

} // namespace vkw

} // namespace sublimation