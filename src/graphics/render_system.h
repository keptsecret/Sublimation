#pragma once

#include <volk.h>

#include <graphics/vulkan/descriptor.h>
#include <graphics/vulkan/pipeline.h>
#include <graphics/vulkan/render_target.h>

#include <memory>
#include <vector>

namespace sublimation {

namespace vkw {
class Texture;
}

class Scene;

struct DepthPrePass : public vkw::Pipeline {
    vkw::RenderTarget renderTarget;

    virtual void setupRenderPass() override;
};

struct ForwardPass : public vkw::Pipeline {
    vkw::RenderTarget renderTarget;

    virtual void setupRenderPass() override;
};

class RenderSystem {
protected:
    RenderSystem() = default;

public:
    ~RenderSystem() = default;
    static RenderSystem* getSingleton();

    void initialize();
    void update(Scene& scene);
    void render();

    uint32_t getFrameIndex() const { return frameIndex; }
    uint32_t getMaxFrameLag() const { return maxFrameLag; }

private:
    void loadShaders();
    void createPipelineLayouts();
    void createDescriptors();
    void createPipelines();
    void createSyncObjects();

    // called when scene changes - descriptor sets
    void updateDescriptorSets(Scene& scene);

    // called every time frame size changes
    void updateRenderSurfaces();
    void cleanupRenderSurfaces();

    uint32_t width = 1920;
    uint32_t height = 1080;
    const uint32_t maxFrameLag = 2;
    bool windowResized = false;

    uint32_t frameIndex = 0;
    uint32_t currentBuffer = 0;

    ForwardPass forwardPass;
    DepthPrePass depthPrePass;

    // Resources
    std::vector<vkw::UniformBuffer> sceneDataBuffers;

    std::vector<VkSemaphore> presentCompleteSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;
    std::vector<VkFence> inFlightFences;
};

} //namespace sublimation
