#include <graphics/vulkan/rendering_device.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "render_target.h"

#include <graphics/vulkan/buffer.h>
#include <graphics/vulkan/descriptor.h>
#include <graphics/vulkan/texture.h>
#include <graphics/vulkan/utils.h>

#include <scene/model.h>

namespace sublimation {

namespace vkw {

void RenderingDevice::initialize() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "New window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    vulkanContext.initialize(window);

    ///< deleted stuff

    commandBufferManager.initialize();

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 }
    };
    descriptorAllocator.initialize(10, sizes);

    // initialize resource pools
    pipelines.reserve(128);
    pipelineLayouts.reserve(128);
    renderPasses.reserve(256);
    bufferObjects.reserve(4096);
    textureObjects.reserve(512);
}

uint32_t RenderingDevice::getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < vulkanContext.memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)) {
            if ((vulkanContext.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
    }

    throw std::runtime_error("ERROR::RenderingDevice:getMemoryType: failed to find suitable memory type!");
}

RenderPass RenderingDevice::createRenderPass(const RenderTarget& target, const VkSubpassDependency& dependency, const std::string& name) {
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = target.getNumColorAttachments(),
        .pColorAttachments = target.getNumColorAttachments() ? target.getColorAttachmentReferences() : nullptr,
        .pResolveAttachments = target.getResolveAttachmentReferences(),
        .pDepthStencilAttachment = target.getDepthStencilReference()
    };

    VkRenderPassCreateInfo renderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = target.getNumAttachmentDescriptions(),
        .pAttachments = target.getAttachmentDescriptions(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkRenderPass renderPass;
    CHECK_VKRESULT(vkCreateRenderPass(vulkanContext.device, &renderPassCreateInfo, nullptr, &renderPass));

    renderPasses.push_back({ renderPass, name });
    return renderPasses.back();
}

bool RenderingDevice::destroyRenderPass(const std::string& name) {
    uint32_t i;
    for (i = 0; i < renderPasses.size(); i++) {
        if (renderPasses[i].name == name) {
            vkDestroyRenderPass(vulkanContext.device, renderPasses[i].renderPass, nullptr);
            break;
        }
    }

    renderPasses.erase(renderPasses.begin() + i);

    return i < renderPasses.size();
}

VkCommandBuffer RenderingDevice::getCommandBuffer(int frameIdx) {
    return commandBufferManager.getCommandBuffer(frameIdx);
}

VkCommandBuffer RenderingDevice::getCommandBufferOneTime(int frameIdx, bool begin) {
    return commandBufferManager.getCommandBufferOneTime(frameIdx, begin);
}

void RenderingDevice::commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType) {
    CHECK_VKRESULT(vkEndCommandBuffer(*buffer));

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = buffer
    };

    VkQueue queue;
    switch (queueType) {
        case VK_QUEUE_GRAPHICS_BIT:
            queue = vulkanContext.graphicsQueue;
            break;
        default:
            queue = VK_NULL_HANDLE;
            break;
    }

    CHECK_VKRESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(queue);
}

VkPipelineLayout RenderingDevice::createPipelineLayout(const Shader& shader) {
    // TODO: set up shader reflection and get descriptor layouts from shader
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ ///< good idea to separate this out
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(shader.layouts.size()),
        .pSetLayouts = shader.layouts.data()
    };

    VkPushConstantRange pushConstants;
    if (shader.pushConstantRange) {
        pushConstants.stageFlags = VK_SHADER_STAGE_ALL;
        pushConstants.offset = 0;
        pushConstants.size = shader.pushConstantRange;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
    }

    VkPipelineLayout pipelineLayout;
    CHECK_VKRESULT(vkCreatePipelineLayout(vulkanContext.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    pipelineLayouts.push_back(pipelineLayout);
    return pipelineLayout;
}

VkPipeline RenderingDevice::createPipeline(const PipelineInfo& pipelineInfo, const Shader& shader) {
    //Shader shader = createShaderFromSPIRV(pipelineInfo.shaders);

    VkPipeline pipeline;

    if (shader.isGraphicsPipeline) {
        auto bindingDescription = Vertex::getBindingDescription(0);
        auto attributeDescriptions = Vertex::getAttributeDescriptions(0);

        VkPipelineVertexInputStateCreateInfo vertexInputState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        ///< ensure viewport and scissors are created
        VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (uint32_t)dynamicStates.size(),
            .pDynamicStates = dynamicStates.data()
        };

        VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        };

        VkPipelineRasterizationStateCreateInfo rasterizationState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = pipelineInfo.rasterizationInfo.polygonMode,
            .cullMode = pipelineInfo.rasterizationInfo.cullMode,
            .frontFace = pipelineInfo.rasterizationInfo.frontFace,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.f
        };

        VkPipelineMultisampleStateCreateInfo multisampleState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = multisampling,
            .sampleShadingEnable = VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = pipelineInfo.depthStencilInfo.depthTestEnable,
            .depthWriteEnable = pipelineInfo.depthStencilInfo.depthWriteEnable,
            .depthCompareOp = pipelineInfo.depthStencilInfo.compareOp,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = pipelineInfo.depthStencilInfo.stencilEnable,
            .front = pipelineInfo.depthStencilInfo.front,
            .back = pipelineInfo.depthStencilInfo.back,
            .minDepthBounds = 0.f,
            .maxDepthBounds = 1.f
        };

        VkPipelineColorBlendStateCreateInfo colorBlendState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = pipelineInfo.blendStateInfo.attachmentCount,
            .pAttachments = pipelineInfo.blendStateInfo.blendStates
        };

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = shader.activeShaders,
            .pStages = shader.shaderStageCreateInfo,
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = pipelineInfo.pipelineLayout,
            .renderPass = pipelineInfo.renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        CHECK_VKRESULT(vkCreateGraphicsPipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
    } else {
        VkComputePipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shader.shaderStageCreateInfo[0],
            .layout = pipelineInfo.pipelineLayout
        };

        CHECK_VKRESULT(vkCreateComputePipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
    }

    pipelines.push_back(pipeline);

    return pipeline;
}

Buffer* RenderingDevice::createBuffer(VkBufferUsageFlags usageFlags, VmaMemoryUsage properties, VkDeviceSize size, const void* data) {
    std::unique_ptr<Buffer> newBuffer = nullptr;

    // check usage flags and create appropriate buffers
    if (usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        newBuffer = std::make_unique<UniformBuffer>(size, data);
    } else if (usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        newBuffer = std::make_unique<StorageBuffer>(size, data);
    } else {
        newBuffer = std::make_unique<Buffer>(size, usageFlags, properties, data);
    }

    bufferObjects.push_back(std::move(newBuffer));
    return newBuffer.get();
}

Texture* RenderingDevice::createTexture(TextureType type, const glm::ivec2& extent, TextureInfo texInfo, VkDeviceSize size, const void* data) {
    std::unique_ptr<Texture> newTexture = nullptr;

    if (type == TEXTURE_DEPTH) {
        newTexture = std::make_unique<TextureDepth>(extent, texInfo.samples);
    } else { // default to 2D texture
        newTexture = std::make_unique<Texture2D>(extent, data, size, texInfo.format, texInfo.layout, texInfo.usage, texInfo.filter,
                texInfo.addressMode, texInfo.samples, texInfo.aniso, texInfo.mipmap);
    }

    textureObjects.push_back(std::move(newTexture));
    return newTexture.get();
}

Texture* RenderingDevice::loadTextureFromFile(const std::string& filename, VkFilter filter, VkSamplerAddressMode addressMode, bool aniso, bool mipmap) {
    textureObjects.push_back(std::make_unique<Texture2D>(filename, filter, addressMode, aniso, mipmap));

    return textureObjects.back().get();
}

Shader RenderingDevice::createShaderFromSPIRV(const ShaderStageInfo& shaderInfo) {
    Shader shader{
        .name = shaderInfo.name,
        .activeShaders = shaderInfo.stageCount,
        .isGraphicsPipeline = true
    };

    for (size_t i = 0; i < shaderInfo.stageCount; i++) {
        ShaderModuleInfo module = shaderInfo.stages[i];

        if (module.stage == VK_SHADER_STAGE_COMPUTE_BIT) {
            shader.isGraphicsPipeline = false;
        }

        VkPipelineShaderStageCreateInfo shaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = module.stage,
            .module = utils::loadShader(module.filepath, vulkanContext.device),
            .pName = "main"
        };

        shader.shaderStageCreateInfo[i] = shaderStageInfo;
    }

    shaders.push_back(shader);

    return shader;
}

void RenderingDevice::windowResizeCallback(GLFWwindow* window, int width, int height) {
    auto rd = (RenderingDevice*)glfwGetWindowUserPointer(window);
    rd->windowResized = true;
}

RenderingDevice::~RenderingDevice() {
    vkDeviceWaitIdle(vulkanContext.device);

    //cleanupRenderArea();
    for (const auto pass : renderPasses) {
        vkDestroyRenderPass(vulkanContext.device, pass.renderPass, nullptr);
    }

    //vkDestroyDescriptorPool(vulkanContext.device, descriptorPool, nullptr);
    //for (size_t i = 0; i < descriptorSetLayouts.size(); i++) {
    //    vkDestroyDescriptorSetLayout(vulkanContext.device, descriptorSetLayouts[i], nullptr);
    //}
    //vkDestroyDescriptorSetLayout(vulkanContext.device, depthPassDescriptorSetLayout, nullptr);
    descriptorAllocator.clearPools();
    descriptorAllocator.destroyPools();

    for (const auto& shader : shaders) {
        for (size_t i = 0; i < shader.activeShaders; i++) {
            vkDestroyShaderModule(vulkanContext.device, shader.shaderStageCreateInfo[i].module, nullptr);
        }
    }
    shaders.clear();

    // should destroy objects
    bufferObjects.clear();
    textureObjects.clear();

    //vkDestroyCommandPool(vulkanContext.device, commandPool, nullptr);
    commandBufferManager.destroy();

    ///< should have same number of pipeline and pipeline layouts (at this point)
    for (size_t i = 0; i < pipelines.size(); i++) {
        vkDestroyPipeline(vulkanContext.device, pipelines[i], nullptr);
        vkDestroyPipelineLayout(vulkanContext.device, pipelineLayouts[i], nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

RenderingDevice* RenderingDevice::getSingleton() {
    static RenderingDevice singleton;
    return &singleton;
}

void RenderingDevice::updateSwapchain(uint32_t* width, uint32_t* height) {
    vulkanContext.updateSwapchain(window);
    *width = vulkanContext.swapChain.width;
    *height = vulkanContext.swapChain.height;
}

} // namespace vkw

} // namespace sublimation