#pragma once

#include <graphics/vulkan/swapchain.h>
#include <graphics/vulkan/texture.h>

#include <memory>

namespace sublimation {

namespace vkw {

struct AttachmentInfo {
    AttachmentInfo(Texture* texture) :
            format(texture->getFormat()),
            isSwapchainResource(false),
            texture(texture) {}

    AttachmentInfo(Swapchain* swapchain) :
            format(swapchain->getFormat()),
            swapchain(swapchain),
            isSwapchainResource(true) {}

    VkFormat format;
    VkAttachmentLoadOp loadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeAction = VK_ATTACHMENT_STORE_OP_STORE;

    bool isSwapchainResource;
    Texture* texture;
    Swapchain* swapchain = nullptr;
};

class RenderTarget {
public:
    RenderTarget() {}

    void addColorAttachment(AttachmentInfo& attachment);
    void addColorResolveAttachment(AttachmentInfo& attachment);
    void setDepthStencilAttachment(AttachmentInfo& attachment);

    void setupFramebuffers(uint32_t count, VkExtent2D extent, VkRenderPass renderPass);
    void destroy();

    uint32_t getNumColorAttachments() const { return static_cast<uint32_t>(colorReferences.size()); }
    bool getHasResolveAttachments() const { return hasResolveAttachments; }
    bool getHasDepthStencil() const { return hasDepthStencil; }

    const VkAttachmentReference* getColorAttachmentReferences() const { return colorReferences.data(); }
    const VkAttachmentReference* getResolveAttachmentReferences() const { return hasResolveAttachments ? resolveReferences.data() : nullptr; }
    const VkAttachmentReference* getDepthStencilReference() const { return hasDepthStencil ? &depthStencilReference : nullptr; }

    uint32_t getNumAttachmentDescriptions() const { return static_cast<uint32_t>(descriptions.size()); }
    const VkAttachmentDescription* getAttachmentDescriptions() const { return descriptions.data(); }

    const VkFramebuffer& getFramebuffer(int index) const { return framebuffers[index]; }

private:
    std::vector<VkFramebuffer> framebuffers;
    VkExtent2D extent;

    uint32_t numAttachments = 0;

    std::vector<VkAttachmentReference> colorReferences;
    std::vector<VkAttachmentReference> resolveReferences;
    VkAttachmentReference depthStencilReference;
    bool hasResolveAttachments = false;
    bool hasDepthStencil = false;

    std::vector<VkAttachmentDescription> descriptions;

    std::vector<AttachmentInfo> attachments;
};

} // namespace vkw

} //namespace sublimation