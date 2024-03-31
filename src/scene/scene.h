#pragma once

#include <graphics/vulkan/buffer.h>

#include <scene/light.h>
#include <scene/model.h>
#include <scene/camera.h>

namespace sublimation {

struct SceneData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 cameraPosition;
    float camNear;
    float camFar;

    DirectionalLight directionalLight{ glm::vec3{ 0, -1, 0 }, glm::vec3{ 1.f }, 0.f };
};

class Scene {
public:
    Scene() {}

    void loadModel(const std::string& filepath, uint32_t postProcessFlags = 0);

    void createDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);
    void addPointLight(glm::vec3 position, glm::vec3 color, float radius, float intensity);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t renderFlags = 0, uint32_t bindImageset = 1);

    void updateSceneBufferData();
    std::weak_ptr<vkw::Buffer> getSceneDataBuffer() const { return sceneDataBuffer; }
    std::weak_ptr<vkw::Buffer> getPointLightsBuffer() const { return pointLightsBuffer; }
    uint32_t getNumLights() const { return pointLights.size(); }

    void unload();

private:
    Camera camera;
    std::unique_ptr<Model> model;

    SceneData sceneData;
    std::vector<PointLight> pointLights;

    std::weak_ptr<vkw::Buffer> sceneDataBuffer;
    std::weak_ptr<vkw::Buffer> pointLightsBuffer;
};

} //namespace sublimation
