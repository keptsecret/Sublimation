#include <graphics/vulkan/descriptor.h>

#include <iostream>
#include <unordered_map>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

VkDescriptorSetLayout DescriptorLayoutBuilder::build(std::string name) {
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data()
    };

    VkDescriptorSetLayout descriptorSetLayout;
    CHECK_VKRESULT(vkCreateDescriptorSetLayout(RenderingDevice::getSingleton()->getDevice(), &layoutCreateInfo, nullptr, &descriptorSetLayout));
    layoutBindings.clear();

    return descriptorSetLayout;
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::addResource(VkDescriptorType type, const uint32_t binding, const VkShaderStageFlagBits stage) {
    layoutBindings.push_back({ .binding = binding,
            .descriptorType = type,
            .descriptorCount = 1,
            .stageFlags = static_cast<VkShaderStageFlags>(stage),
            .pImmutableSamplers = nullptr });

    return *this;
}

void DescriptorAllocator::initialize(uint32_t initialSets, std::span<PoolSizeRatio> poolRatios) {
    ratios.clear();

    for (auto r : poolRatios) {
        ratios.push_back(r);
    }

    VkDescriptorPool pool = createPool(initialSets, poolRatios);
    setsPerPool = initialSets * 1.5f;
    readyPools.push_back(pool);
}

void DescriptorAllocator::clearPools() {
    VkDevice device = RenderingDevice::getSingleton()->getDevice();

    for (auto pool : readyPools) {
        vkResetDescriptorPool(device, pool, 0);
    }
    for (auto pool : fullPools) {
        vkResetDescriptorPool(device, pool, 0);
        readyPools.push_back(pool);
    }
    fullPools.clear();
}

void DescriptorAllocator::destroyPools() {
    VkDevice device = RenderingDevice::getSingleton()->getDevice();

    for (auto pool : readyPools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    readyPools.clear();
    for (auto pool : fullPools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout, void* pNext) {
    VkDescriptorPool pool = getPool();

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet descriptorSet;
    VkDevice device = RenderingDevice::getSingleton()->getDevice();
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    // pool is full, allocate new
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        fullPools.push_back(pool);

        pool = getPool();
        allocInfo.descriptorPool = pool;

        CHECK_VKRESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    }

    readyPools.push_back(pool);
    return descriptorSet;
}

VkDescriptorPool DescriptorAllocator::getPool() {
    VkDescriptorPool pool;
    if (readyPools.size() != 0) {
        pool = readyPools.back();
        readyPools.pop_back();
    } else {
        pool = createPool(setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5f;
        if (setsPerPool > 4092)
            setsPerPool = 4092;
    }

    return pool;
}

VkDescriptorPool DescriptorAllocator::createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio& ratio : poolRatios) {
        poolSizes.push_back({ .type = ratio.type, .descriptorCount = uint32_t(ratio.ratio * setCount) });
    }

    VkDescriptorPoolCreateInfo poolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = setCount,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data()
    };

    VkDescriptorPool pool;
    CHECK_VKRESULT(vkCreateDescriptorPool(RenderingDevice::getSingleton()->getDevice(), &poolCreateInfo, nullptr, &pool));
}

void DescriptorWriter::bindImage(uint32_t binding, Texture* texture, VkDescriptorType type) {
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = texture->getSampler(),
            .imageView = texture->getImageView(),
            .imageLayout = texture->getLayout() });

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info
    };

    descriptorWrites.push_back(write);
}

void DescriptorWriter::bindBuffer(uint32_t binding, Buffer* buffer, VkDescriptorType type) {
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
            .buffer = buffer->getBuffer(),
            .offset = 0,
            .range = buffer->getSize() });

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &info
    };

    descriptorWrites.push_back(write);
}

void DescriptorWriter::writeSet(VkDescriptorSet descriptorSet)
{
    for (auto& write : descriptorWrites) {
        write.dstSet = descriptorSet;
    }

    vkUpdateDescriptorSets(RenderingDevice::getSingleton()->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    imageInfos.clear();
    bufferInfos.clear();
    descriptorWrites.clear();
}

} //namespace vkw

} //namespace sublimation
