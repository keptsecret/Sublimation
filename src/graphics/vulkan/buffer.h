#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

namespace sublimation {

namespace vkw {

class Buffer {
public:
    Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage properties, const void* data = nullptr);
    virtual ~Buffer();

    const VkBuffer& getBuffer() const { return buffer; }
    VkDeviceSize getSize() const { return allocInfo.size; }

protected:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation;
    VmaAllocationInfo allocInfo;
};

class UniformBuffer : public Buffer {
public:
    UniformBuffer(VkDeviceSize size, const void* data = nullptr);

    void update(const void* data);

private:
    void* mapped = nullptr;
};

class StorageBuffer : public Buffer {
public:
    StorageBuffer(VkDeviceSize size, const void* data = nullptr);

    void update(const void* data);

private:
    void* mapped = nullptr;
};

} // namespace vkw

} //namespace sublimation