#include <graphics/vulkan/descriptor.h>

#include <iostream>
#include <unordered_map>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

Descriptor::Descriptor(std::string&& name, std::vector<VkDescriptorSetLayoutBinding>&& layoutBindings, std::vector<VkWriteDescriptorSet>&& writeSets) :
        name(name), descriptorLayoutBindings(layoutBindings), writeDescriptorSets(writeSets) {
    for (size_t i = 0; i < layoutBindings.size(); i++) {
        if (layoutBindings[i].descriptorType != writeSets[i].descriptorType) {
            std::cerr << "VkDescriptorType mismatch for descriptor set " << name << "\n";
        }
    }

    const VkDevice device = RenderingDevice::getSingleton()->getDevice();

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data()
    };

    CHECK_VKRESULT(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout));

    std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;
    for (const auto& binding : layoutBindings) {
        descriptorTypes[binding.descriptorType]++;
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorTypes.size());

    for (const auto& type : descriptorTypes) {
        poolSizes.push_back({ type.first, type.second });
    }

    VkDescriptorPoolCreateInfo poolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 16, // TODO: temporary value, hope we don't have more than 16 descriptor sets per pool
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    CHECK_VKRESULT(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool));

    VkDescriptorSetAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

    descriptorSets.resize(1);
    CHECK_VKRESULT(vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()));

    for (uint32_t i = 0; i < writeSets.size(); i++) {
        writeSets[i].dstBinding = i;
        writeSets[i].dstSet = descriptorSets[0];
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

Descriptor::Descriptor(Descriptor&& other) noexcept {
    name = std::move(other.name);
    descriptorPool = std::exchange(other.descriptorPool, nullptr);
    descriptorSetLayout = std::exchange(other.descriptorSetLayout,  nullptr);
    descriptorLayoutBindings = std::move(other.descriptorLayoutBindings);
    writeDescriptorSets = std::move(other.writeDescriptorSets);
    descriptorSets = std::move(other.descriptorSets);
}

Descriptor::~Descriptor() {
    const VkDevice device = RenderingDevice::getSingleton()->getDevice();
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

Descriptor DescriptorBuilder::build(std::string name) {
    Descriptor newDescriptor(std::move(name), std::move(layoutBindings), std::move(writeDescriptorSets));

    descriptorBufferInfos.clear();
    descriptorImageInfos.clear();

    return std::move(newDescriptor);
}

template <typename T>
DescriptorBuilder& DescriptorBuilder::addUniformBuffer(UniformBuffer& buffer, const uint32_t binding, const VkShaderStageFlagBits stage) {
    layoutBindings.push_back({ .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = static_cast<VkShaderStageFlags>(stage),
            .pImmutableSamplers = nullptr });

    descriptorBufferInfos.push_back({ .buffer = buffer.getBuffer(),
            .offset = 0,
            .range = sizeof(T) });

    writeDescriptorSets.push_back({ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &descriptorBufferInfos.back() });

    return *this;
}

template <typename T>
DescriptorBuilder& DescriptorBuilder::addStorageBuffer(StorageBuffer& buffer, const uint32_t binding, const VkShaderStageFlagBits stage) {
    layoutBindings.push_back({ .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = static_cast<VkShaderStageFlags>(stage),
            .pImmutableSamplers = nullptr });

    descriptorBufferInfos.push_back({ .buffer = buffer.getBuffer(),
            .offset = 0,
            .range = sizeof(T) });

    writeDescriptorSets.push_back({ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &descriptorBufferInfos.back() });

    return *this;
}

template <typename T>
DescriptorBuilder& DescriptorBuilder::addCombinedImageSampler(Texture& texture, const uint32_t binding, const VkShaderStageFlagBits stage) {
    layoutBindings.push_back({ .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = static_cast<VkShaderStageFlags>(stage),
            .pImmutableSamplers = nullptr });

    descriptorImageInfos.push_back({ .sampler = texture.getSampler(),
            .imageView = texture.getImageView(),
            .imageLayout = texture.getLayout() });

    writeDescriptorSets.push_back({ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfos.back() });

    return *this;
}
} //namespace vkw

} //namespace sublimation
