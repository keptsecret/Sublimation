#include <graphics/vulkan/buffer.h>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage properties, const void* data) {
    RenderingDevice* rd = RenderingDevice::getSingleton();
    VkDevice device = rd->getDevice();

    VkBufferCreateInfo bufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocCreateInfo{
        .usage = properties
    };

    CHECK_VKRESULT(vmaCreateBuffer(rd->getAllocator(), &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo));

    if (data != nullptr) {
        void* mapped;
        vmaMapMemory(rd->getAllocator(), allocation, &mapped);
        memcpy(mapped, data, (size_t)bufferCreateInfo.size);
        vmaUnmapMemory(rd->getAllocator(), allocation);
    }
}

Buffer::~Buffer() {
    vmaDestroyBuffer(RenderingDevice::getSingleton()->getAllocator(), buffer, allocation);
}

UniformBuffer::UniformBuffer(VkDeviceSize size, const void* data) :
        Buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr) {
    vmaMapMemory(RenderingDevice::getSingleton()->getAllocator(), allocation, &mapped);
    if (data != nullptr) {
        update(data);
    }
}

void UniformBuffer::update(const void* data) {
    memcpy(mapped, data, allocInfo.size);
}

StorageBuffer::StorageBuffer(VkDeviceSize size, const void* data) :
        Buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr) {
    vmaMapMemory(RenderingDevice::getSingleton()->getAllocator(), allocation, &mapped);
    if (data != nullptr) {
        update(data);
    }
}
void StorageBuffer::update(const void* data) {
    memcpy(mapped, data, allocInfo.size);
}

} // namespace vkw

} //namespace sublimation