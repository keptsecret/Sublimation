#pragma once

#include <string>
#include <vector>
#include <volk.h>

#include <graphics/vulkan/buffer.h>
#include <graphics/vulkan/texture.h>

namespace sublimation {

namespace vkw {

class Descriptor {
public:
    Descriptor(std::string&& name, std::vector<VkDescriptorSetLayoutBinding>&& layoutBindings, std::vector<VkWriteDescriptorSet>&& writeSets);
    Descriptor(Descriptor&& other) noexcept;
    Descriptor(const Descriptor& other) = delete;

    ~Descriptor();

    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }
    const VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    const std::vector<VkDescriptorSetLayoutBinding>& getDescriptorSetLayoutBindings() const { return descriptorLayoutBindings; }

private:
    std::string name;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    // temporary?
    std::vector<VkDescriptorSetLayoutBinding> descriptorLayoutBindings;
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    std::vector<VkDescriptorSet> descriptorSets;
};

class DescriptorBuilder {
public:
    explicit DescriptorBuilder() = default;
    DescriptorBuilder(DescriptorBuilder&& other) = delete;
    DescriptorBuilder(const DescriptorBuilder& other) = delete;

    ~DescriptorBuilder() = default;

    template <typename T>
    DescriptorBuilder& addUniformBuffer(UniformBuffer& buffer, const uint32_t binding, const VkShaderStageFlagBits stage);
    template <typename T>
    DescriptorBuilder& addStorageBuffer(StorageBuffer& buffer, const uint32_t binding, const VkShaderStageFlagBits stage);
    template <typename T>
    DescriptorBuilder& addCombinedImageSampler(Texture& buffer, const uint32_t binding, const VkShaderStageFlagBits stage);

    [[nodiscard]] Descriptor build(std::string name);

private:
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
    std::vector<VkDescriptorImageInfo> descriptorImageInfos;
};

} //namespace vkw

} //namespace sublimation
