#include <graphics/vulkan/render_target.h>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

void RenderTarget::addColorAttachment(AttachmentInfo& attachment) {
    attachments.push_back(attachment);

    VkAttachmentReference reference{
        .attachment = numAttachments,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    colorReferences.push_back(reference);

    VkAttachmentDescription description{
        .loadOp = attachment.loadAction,
        .storeOp = attachment.storeAction,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    description.format = attachment.texture->getFormat();
    description.samples = attachment.texture->getSamples();

    descriptions.push_back(description);

    numAttachments++;
}

void RenderTarget::addColorResolveAttachment(AttachmentInfo& attachment) {
    attachments.push_back(attachment);

    VkAttachmentReference reference{
        .attachment = numAttachments,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    resolveReferences.push_back(reference);

    VkAttachmentDescription description{
        .format = attachment.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = attachment.loadAction,
        .storeOp = attachment.storeAction,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    descriptions.push_back(description);

    hasResolveAttachments = true;
    numAttachments++;
}

void RenderTarget::setDepthStencilAttachment(AttachmentInfo& attachment) {
    attachments.push_back(attachment);

    VkAttachmentReference reference{
        .attachment = numAttachments,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    depthStencilReference = reference;

    VkAttachmentDescription description{
        .loadOp = attachment.loadAction,
        .storeOp = attachment.storeAction,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    description.format = attachment.texture->getFormat();
    description.samples = attachment.texture->getSamples();

    if (attachment.storeAction == VK_ATTACHMENT_STORE_OP_STORE && attachment.loadAction == VK_ATTACHMENT_LOAD_OP_CLEAR) {
        // are we writing to depth texture?
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else if (attachment.storeAction == VK_ATTACHMENT_STORE_OP_STORE && attachment.loadAction == VK_ATTACHMENT_LOAD_OP_LOAD) {
        description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    descriptions.push_back(description);

    hasDepthStencil = true;
    numAttachments++;
}

void RenderTarget::setupFramebuffers(uint32_t count, VkExtent2D ext, VkRenderPass renderPass) {
    extent = ext;

    // If there is a swapchain attachment, count should be the number of swapchain images
    framebuffers.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        std::vector<VkImageView> viewAttachments(attachments.size());
        for (uint32_t j = 0; j < attachments.size(); j++) {
            if (attachments[j].isSwapchainResource) {
                viewAttachments[j] = attachments[j].swapchain->getImageView(i);
            } else {
                viewAttachments[j] = attachments[j].texture->getImageView();
            }
        }

        VkFramebufferCreateInfo framebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = (uint32_t)viewAttachments.size(),
            .pAttachments = viewAttachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };

        CHECK_VKRESULT(vkCreateFramebuffer(RenderingDevice::getSingleton()->getDevice(), &framebufferCreateInfo, nullptr, &framebuffers[i]));
    }
}

void RenderTarget::destroy() {
    attachments.clear();

    colorReferences.clear();
    resolveReferences.clear();
    descriptions.clear();

    hasResolveAttachments = false;
    hasDepthStencil = false;

    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(RenderingDevice::getSingleton()->getDevice(), framebuffer, nullptr);
    }
    numAttachments = 0;
}

} // namespace vkw

} //namespace sublimation