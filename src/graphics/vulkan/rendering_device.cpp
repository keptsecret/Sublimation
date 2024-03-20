#include <graphics/vulkan/rendering_device.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <graphics/vulkan/utils.h>

#include <graphics/vulkan/buffer.h>
#include <graphics/vulkan/texture.h>
#include <graphics/vulkan/descriptor.h>

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

    // initialize resource pools
    pipelines.reserve(128);
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

    vkFreeCommandBuffers(vulkanContext.device, commandPool, 1, buffer);
}

VkPipeline RenderingDevice::createPipeline(const PipelineInfo& pipelineInfo, const std::vector<Descriptor>& descriptors) {
    Shader shader = createShaderFromSPIRV(pipelineInfo.shaders);

    // TODO: set up shader reflection and get descriptor layouts from shader
    std::vector<VkDescriptorSetLayout> layouts(descriptors.size());
    for (size_t i = 0; i < descriptors.size(); i++) {
        layouts[i] = descriptors[i].getDescriptorSetLayout();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ ///< good idea to separate this out
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    VkPushConstantRange pushConstants;
    if (shader.pushConstantRange) {
        pushConstants.stageFlags = VK_SHADER_STAGE_ALL;
        pushConstants.offset = 0;
        pushConstants.size = shader.pushConstantRange;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
    }

    CHECK_VKRESULT(vkCreatePipelineLayout(vulkanContext.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

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
            .minDepthBounds = 0.f,
            .maxDepthBounds = 1.f,
            .front = pipelineInfo.depthStencilInfo.front,
            .back = pipelineInfo.depthStencilInfo.back
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
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        CHECK_VKRESULT(vkCreateGraphicsPipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
    } else {
        VkComputePipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shader.shaderStageCreateInfo[0],
            .layout = pipelineLayout
        };

        CHECK_VKRESULT(vkCreateComputePipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
    }

    pipelines.push_back(pipeline);

    return pipeline;
}

std::weak_ptr<Buffer> RenderingDevice::createBuffer(VkBufferUsageFlags usageFlags, VmaMemoryUsage properties, VkDeviceSize size, const void* data) {
    std::shared_ptr<Buffer> newBuffer = nullptr;

    // check usage flags and create appropriate buffers
    if (usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        newBuffer = std::make_shared<UniformBuffer>(size, data);
    } else if (usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        newBuffer = std::make_shared<StorageBuffer>(size, data);
    } else {
        newBuffer = std::make_shared<Buffer>(size, usageFlags, properties, data);
    }

    bufferObjects.push_back(newBuffer);
    return std::weak_ptr<Buffer>(newBuffer);
}

std::weak_ptr<Texture> RenderingDevice::createTexture(TextureType type, const glm::ivec2& extent, TextureInfo texInfo) {
    std::shared_ptr<Texture> newTexture = nullptr;

    if (type == TEXTURE_DEPTH) {
        newTexture = std::make_shared<TextureDepth>(extent, texInfo.samples);
    } else { // default to 2D texture
        newTexture = std::make_shared<Texture2D>(extent, nullptr, 0, texInfo.format, texInfo.layout, texInfo.usage, texInfo.filter,
                texInfo.addressMode, texInfo.samples, texInfo.aniso, texInfo.mipmap);
    }

    textureObjects.push_back(newTexture);
    return { newTexture };
}

std::weak_ptr<Texture> RenderingDevice::loadTextureFromFile(const std::string& filename, VkFilter filter, VkSamplerAddressMode addressMode, bool aniso, bool mipmap) {
    textureObjects.push_back(std::make_shared<Texture2D>(filename, filter, addressMode, aniso, mipmap));

    return { textureObjects.back() };
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

    vkDestroyDescriptorPool(vulkanContext.device, descriptorPool, nullptr);
    for (size_t i = 0; i < descriptorSetLayouts.size(); i++) {
        vkDestroyDescriptorSetLayout(vulkanContext.device, descriptorSetLayouts[i], nullptr);
    }
    vkDestroyDescriptorSetLayout(vulkanContext.device, depthPassDescriptorSetLayout, nullptr);

    for (size_t i = 0; i < maxFrameLag; i++) {
        vkDestroySemaphore(vulkanContext.device, presentCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanContext.device, renderCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanContext.device, depthPrePassCompleteSemaphores[i], nullptr);
        vkDestroyFence(vulkanContext.device, inFlightFences[i], nullptr);
        vkDestroyFence(vulkanContext.device, depthPassFences[i], nullptr);
    }

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
    vkDestroyPipeline(vulkanContext.device, renderPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanContext.device, pipelineLayout, nullptr);

    vkDestroyPipeline(vulkanContext.device, depthPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanContext.device, depthPipelineLayout, nullptr);

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