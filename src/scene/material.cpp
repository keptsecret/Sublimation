#include <scene/material.h>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

Material::Material() :
        auxUbo(sizeof(MaterialAux)) {
}

void Material::apply() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

    // Fill in missing textures with constant values (1x1 textures)
    const std::array<glm::vec4, 5> values{ albedo, glm::vec4{ metallicRoughnessOcclusionFactor.x },
        glm::vec4{ metallicRoughnessOcclusionFactor.y }, glm::vec4{ metallicRoughnessOcclusionFactor.z }, glm::vec4{ 0 } };

    for (size_t i = 0; i < 5; i++) {
        if (!textures[i].isActive()) {
            textures[i].constant = values[i];

            textures[i].texture = rd->createTexture(vkw::TEXTURE_2D, { 1, 1 },
                    {
                            VK_FORMAT_R32G32B32A32_SFLOAT,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                            VK_FILTER_NEAREST,
                            VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VK_SAMPLE_COUNT_1_BIT,
                    },
                    4 * sizeof(float), glm::value_ptr(textures[i].constant));
        }
    }

    auxUbo.update(&aux);
}

void Material::updateDescriptorSets(const VkDescriptorSetLayout& layout) {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

    // if descriptor not allocated, do that first
    if (descriptorSet == VK_NULL_HANDLE) {
        vkw::DescriptorAllocator allocator = rd->getDescriptorAllocator();

        descriptorSet = allocator.allocate(layout);
    }
    
    // write descriptors
    {
        vkw::DescriptorWriter writer;

        writer.bindBuffer(0, &auxUbo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        for (uint32_t i = 0; i < textures.size(); i++) {
            writer.bindImage(i + 1, textures[i].texture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
        writer.writeSet(descriptorSet);
    }
}

} //namespace sublimation