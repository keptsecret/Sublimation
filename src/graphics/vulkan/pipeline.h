#pragma once

#include <volk.h>

namespace sublimation {

namespace vkw {

struct RasterizationInfo {
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
};

struct DepthStencilInfo {
    VkStencilOpState front = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_ALWAYS
    };
    VkStencilOpState back = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_ALWAYS
    };

    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;

    uint8_t depthTestEnable = VK_TRUE;
    uint8_t depthWriteEnable = VK_TRUE;
    uint8_t stencilEnable = VK_TRUE;
};

struct BlendStateInfo {
    VkPipelineColorBlendAttachmentState blendStates[8];
    uint32_t attachmentCount = 0;
};

struct Shader {
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo[5];
    std::string name;

    uint32_t activeShaders = 0;
    bool isGraphicsPipeline = true;

    // move into reflection results
    //vector of Descriptor descriptors
    uint32_t pushConstantRange = 0;
};

struct ShaderModuleInfo {
    std::string filepath;
    VkShaderStageFlagBits stage;
};

struct ShaderStageInfo {
    ShaderModuleInfo stages[5];

    uint32_t stageCount = 0;
    std::string name;
};

struct PipelineInfo {
    RasterizationInfo rasterizationInfo;
    DepthStencilInfo depthStencilInfo;
    BlendStateInfo blendStateInfo;

    ShaderStageInfo shaders;
};

} //namespace vkw

} //namespace sublimation