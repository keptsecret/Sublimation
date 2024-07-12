#pragma once

#include <string>
#include <vector>
#include <span>
#include <deque>
#include <volk.h>

#include <graphics/vulkan/buffer.h>
#include <graphics/vulkan/texture.h>

namespace sublimation {

namespace vkw {

class DescriptorLayoutBuilder {
public:
    explicit DescriptorLayoutBuilder() = default;
    DescriptorLayoutBuilder(DescriptorLayoutBuilder&& other) = delete;
    DescriptorLayoutBuilder(const DescriptorLayoutBuilder& other) = delete;

    ~DescriptorLayoutBuilder() = default;

    DescriptorLayoutBuilder& addResource(VkDescriptorType type, const uint32_t binding, const VkShaderStageFlagBits stage);

    [[nodiscard]] VkDescriptorSetLayout build(std::string name);

private:
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
};

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void initialize(uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void clearPools();
    void destroyPools();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
    VkDescriptorPool getPool();
	VkDescriptorPool createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools;
	std::vector<VkDescriptorPool> readyPools;
	uint32_t setsPerPool;
};

struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    void bindImage(uint32_t binding, Texture* texture, VkDescriptorType type);
    void bindBuffer(uint32_t binding, Buffer* buffer, VkDescriptorType type);

    void writeSet(VkDescriptorSet descriptorSet);
};

} //namespace vkw

} //namespace sublimation
