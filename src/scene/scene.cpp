#include "graphics/vulkan/rendering_device.h"

#include <scene/scene.h>

#include <assimp/Importer.hpp>
#include <iostream>

namespace sublimation {

void Scene::loadModel(const std::string& filepath, uint32_t postProcessFlags) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, postProcessFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::Model:loadFromFile: " << importer.GetErrorString() << '\n';
        return;
    }

    Model* newmodel = new Model();
    newmodel->loadFromAiScene(scene, filepath);
    model = std::unique_ptr<Model>(newmodel);
}

void Scene::createDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) {
    sceneData.directionalLight.setDirection(direction);
    sceneData.directionalLight.setColor(color);
    sceneData.directionalLight.setIntensity(intensity);
}

void Scene::addPointLight(glm::vec3 position, glm::vec3 color, float radius, float intensity) {
    pointLights.emplace_back(position, color, radius, intensity);
}

void Scene::updateSceneBufferData() {
    //directionalLight.preprocess(bounds.center(), bounds.radius());

    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    if (sceneDataBuffer.expired()) {
        sceneDataBuffer = rd->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(SceneData));
    }

    if (pointLightsBuffer.expired()) {
        pointLightsBuffer = rd->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(PointLight) * pointLights.size());
    }

    sceneData.projection = camera.getProjectionTransform();
    sceneData.view = camera.getViewTransform();
    sceneData.cameraPosition.x = camera.position.x;
    sceneData.cameraPosition.y = camera.position.y;
    sceneData.cameraPosition.z = camera.position.z;
    sceneData.camNear = camera.near_plane;
    sceneData.camFar = camera.far_plane;

    if (auto buffer = std::static_pointer_cast<vkw::UniformBuffer>(sceneDataBuffer.lock())) {
        buffer->update(&sceneData);
    }

    if (auto buffer = std::static_pointer_cast<vkw::StorageBuffer>(pointLightsBuffer.lock())) {
        buffer->update(pointLights.data());
    }
}

void Scene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t renderFlags, uint32_t bindImageset) {
    model->draw(commandBuffer, pipelineLayout, renderFlags, bindImageset);
}

void Scene::unload() {
    model.reset();
}

} //namespace sublimation