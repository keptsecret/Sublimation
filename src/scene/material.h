#pragma once

#include <graphics/vulkan/buffer.h>
#include <graphics/vulkan/texture.h>
#include <graphics/vulkan/descriptor.h>

#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <array>

namespace sublimation {

struct Texture {
    std::weak_ptr<vkw::Texture> texture;

    std::string filepath;
    glm::vec4 constant{0, 0, 0, -1};

    bool isActive() const { return texture.expired(); }
    bool isConstantValue() const { return constant.w < 0.f; }
};

struct MaterialAux {
    uint32_t normalMapMode = 0; // 0 = use vertex normals, 1 = use normal map, 2 = use bump map
    uint32_t roughnessGlossyMode = 0; // 0 = roughness, 1 = glossy (value inverted  in shader)
};

class Material {
public:
    Material();
    ~Material() = default;

    glm::vec4 albedo{ 0.8f, 0.8f, 0.8f , 1.f};
    glm::vec4 metallicRoughnessOcclusionFactor;

    std::array<Texture, 5> textures;    //< in order: [0]albedo, [1]metallic, [2]roughness, [3]ao, [4]normal

    MaterialAux aux;
    vkw::UniformBuffer auxUbo;

    void apply();

    vkw::Descriptor descriptorSet;
    void updateDescriptors();
};

} //namespace sublimation
