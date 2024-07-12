#pragma once

#include <graphics/vulkan/buffer.h>

#include <scene/camera.h>
#include <scene/light.h>
#include <scene/model.h>

namespace sublimation {

struct SceneData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 cameraPosition;
    float camNear;
    float camFar;
};

struct GpuSceneData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 cameraPosition;
    float camNear;
    float camFar;
    uint32_t pad0;
    uint32_t pad1;

    glm::vec4 lightDirection;
    glm::vec4 lightColor;
    float lightIntensity;
    uint32_t pad2;
    uint32_t pad3;
    uint32_t pad4;
};

class Scene {
public:
    Scene() = default;

    void loadModel(const std::string& filepath, uint32_t postProcessFlags = 0);

    void createDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);
    void addPointLight(glm::vec3 position, glm::vec3 color, float radius, float intensity);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t renderFlags = 0, uint32_t bindImageset = 1);

    void updateSceneDescriptors(const VkDescriptorSetLayout& layout);
    void updateSceneBufferData();
    vkw::Buffer* getSceneDataBuffer() const { return sceneDataBuffer; }
    vkw::Buffer* getPointLightsBuffer() const { return pointLightsBuffer; }
    uint32_t getNumLights() const { return pointLights.size(); }

    Camera* getCamera() { return &camera; }

    void unload();

private:
    Camera camera;
    std::unique_ptr<Model> model;

    GpuSceneData sceneData;
    DirectionalLight directionalLight{ glm::vec3{ 0, -1, 0 }, glm::vec3{ 1.f }, 0.f };
    std::vector<PointLight> pointLights;

    vkw::UniformBuffer* sceneDataBuffer = nullptr;
    vkw::StorageBuffer* pointLightsBuffer = nullptr;
};

} //namespace sublimation
