#include <graphics/vulkan/command_buffer.h>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

struct CommandPoolContainer {
    uint32_t index = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers; // 1 buffer per pool for now
};

void CommandBufferManager::initialize() {
    RenderingDevice* rd = RenderingDevice::getSingleton();

    const uint32_t numPools = rd->getSwapChain().getImageCount();
    commandPools.resize(numPools);

    for (size_t i = 0; i < numPools; i++) {
        VkCommandPoolCreateInfo commandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = rd->getGraphicsQueueFamily()
        };

        CHECK_VKRESULT(vkCreateCommandPool(rd->getDevice(), &commandPoolCreateInfo, nullptr, &commandPools[i].commandPool));

        commandPools[i].commandBuffers.resize(1);

        VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPools[i].commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        CHECK_VKRESULT(vkAllocateCommandBuffers(rd->getDevice(), &allocateInfo, commandPools[i].commandBuffers.data()));
    }
}

void CommandBufferManager::destroy() {
    RenderingDevice* rd = RenderingDevice::getSingleton();

    const uint32_t numPools = rd->getSwapChain().getImageCount();

    for (size_t i = 0; i < numPools; i++) {
        vkDestroyCommandPool(rd->getDevice(), commandPools[i].commandPool, nullptr);
    }
}

void CommandBufferManager::resetPool(uint32_t frameIdx) {
    RenderingDevice* rd = RenderingDevice::getSingleton();
    vkResetCommandPool(rd->getDevice(), commandPools[frameIdx].commandPool, 0);

    commandPools[frameIdx].index = 0;
}

const VkCommandPool& CommandBufferManager::getCommandPool(uint32_t frameIdx) {
    return commandPools[frameIdx].commandPool;
}

VkCommandBuffer CommandBufferManager::getCommandBuffer(uint32_t frameIdx) {
    CommandPoolContainer pool = commandPools[frameIdx];

    if (pool.index < pool.commandBuffers.size()) {
        auto buffer = pool.commandBuffers[pool.index++];

        return buffer;
    } else {
        RenderingDevice* rd = RenderingDevice::getSingleton();

        VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer buffer;
        CHECK_VKRESULT(vkAllocateCommandBuffers(rd->getDevice(), &allocateInfo, &buffer));

        pool.commandBuffers.push_back(buffer);
        pool.index++;

        return buffer;
    }
}

VkCommandBuffer CommandBufferManager::getCommandBufferOneTime(uint32_t frameIdx, bool begin) {
    // creates new command buffer and should be freed immediately after use
    RenderingDevice* rd = RenderingDevice::getSingleton();

    auto buffer = getCommandBuffer(frameIdx);

    if (begin) {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        CHECK_VKRESULT(vkBeginCommandBuffer(buffer, &beginInfo));
    }

    return buffer;
}

} //namespace vkw

} //namespace sublimation
