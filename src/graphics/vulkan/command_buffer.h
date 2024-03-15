#pragma once

#include <vector>
#include <volk.h>

namespace sublimation {

namespace vkw {

struct CommandPoolContainer;

class CommandBufferManager {
public:
    CommandBufferManager() = default;

    void initialize();
    void destroy();

    void resetPool(uint32_t frameIdx);

    const VkCommandPool& getCommandPool(uint32_t frameIdx);
    VkCommandBuffer getCommandBuffer(uint32_t frameIdx);
    VkCommandBuffer getCommandBufferOneTime(uint32_t frameIdx, bool begin = false);

private:
    // 1 command pool per thread per swapchain frame
    std::vector<CommandPoolContainer> commandPools;
};

} //namespace vkw

} //namespace sublimation