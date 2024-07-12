#include "scene/scene.h"

#include <graphics/render_system.h>

#include <core/engine.h>
#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>
#include <graphics/vulkan/descriptor.h>
#include <graphics/vulkan/texture.h>

namespace sublimation {

void DepthPrePass::setupRenderPass() {
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    renderPass = rd->createRenderPass(renderTarget, dependency, "depth");
}

void ForwardPass::setupRenderPass() {
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
    };

    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    renderPass = rd->createRenderPass(renderTarget, dependency, "forward");
}

RenderSystem* RenderSystem::getSingleton() {
    static RenderSystem singleton;
    return &singleton;
}

void RenderSystem::initialize() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    rd->initialize();

    // load shaders - descriptor layout with reflection
    loadShaders();
    // setup pipeline layouts
    createPipelineLayouts();

    // set up descriptors
    createDescriptors();

    updateRenderSurfaces();

    // create render passes
    forwardPass.setupRenderPass();
    depthPrePass.setupRenderPass();

    // create pipelines
    createPipelines();

    // write descriptors

    createSyncObjects();

    for (int i = 0; i < maxFrameLag; i++) {
		sceneDataBuffers.emplace_back(sizeof(GpuSceneData));
	}
}

void RenderSystem::loadShaders() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

    vkw::ShaderStageInfo forwardShaderInfo = {};
    forwardShaderInfo.stages[0].filepath = "forward.vert.glsl";
    forwardShaderInfo.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    forwardShaderInfo.stages[1].filepath = "forward.frag.glsl";
    forwardShaderInfo.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    forwardShaderInfo.stageCount = 2;
    forwardShaderInfo.name = "forward";

    forwardPass.shader = rd->createShaderFromSPIRV(forwardShaderInfo);

    vkw::ShaderStageInfo depthShaderInfo = {};
    depthShaderInfo.stages[0].filepath = "depth.vert.glsl";
    depthShaderInfo.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    depthShaderInfo.stageCount = 1;
    depthShaderInfo.name = "depth";

    depthPrePass.shader = rd->createShaderFromSPIRV(depthShaderInfo);


    // layout bindings
    // TODO: move when reflection is done
    VkDescriptorSetLayoutBinding globalsLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding directionalLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = { globalsLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		    .bindingCount = (uint32_t)bindings.size(),
		    .pBindings = bindings.data()
	    };

	    VkDescriptorSetLayout layout;
	    CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &layoutCreateInfo, nullptr, &layout));
        depthPrePass.shader.layouts.push_back(layout);
    }

    {
        globalsLayoutBinding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { globalsLayoutBinding, directionalLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		    .bindingCount = (uint32_t)bindings.size(),
		    .pBindings = bindings.data()
	    };

	    VkDescriptorSetLayout layout;
	    CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &layoutCreateInfo, nullptr, &layout));
        forwardPass.shader.layouts.push_back(layout);
    }

    VkDescriptorSetLayoutBinding matauxLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding albedoLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding metallicLayoutBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding roughLayoutBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding ambientLayoutBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding normalLayoutBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

    {
	    std::vector<VkDescriptorSetLayoutBinding> bindings = { matauxLayoutBinding, albedoLayoutBinding, metallicLayoutBinding, roughLayoutBinding, ambientLayoutBinding, normalLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		    .bindingCount = (uint32_t)bindings.size(),
		    .pBindings = bindings.data()
	    };

	    VkDescriptorSetLayout layout;
	    CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &layoutCreateInfo, nullptr, &layout));
	    forwardPass.shader.layouts.push_back(layout);
    }
}

void RenderSystem::createPipelineLayouts() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

    forwardPass.pipelineLayout = rd->createPipelineLayout(forwardPass.shader);
    depthPrePass.pipelineLayout = rd->createPipelineLayout(depthPrePass.shader);
}

void RenderSystem::createDescriptors() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    vkw::DescriptorAllocator allocator = rd->getDescriptorAllocator();

    for (uint32_t i = 0; i < maxFrameLag; i++) {
        forwardPass.descriptors.push_back(allocator.allocate(forwardPass.shader.layouts[0]));
        depthPrePass.descriptors.push_back(allocator.allocate(depthPrePass.shader.layouts[0]));
    }

    //for (const auto& layout : forwardPass.shader.layouts) {
    //    for (uint32_t i = 0; i < maxFrameLag; i++) {
    //        forwardPass.descriptors.push_back(allocator.allocate(layout));
    //    }
    //}

    //for (const auto& layout : depthPrePass.shader.layouts) {
    //    for (uint32_t i = 0; i < maxFrameLag; i++) {
    //        depthPrePass.descriptors.push_back(allocator.allocate(layout));
    //    }
    //}
}

void RenderSystem::createPipelines() {
    vkw::RasterizationInfo rasterInfo{
        .cullMode = VK_CULL_MODE_BACK_BIT
    };

    vkw::DepthStencilInfo depthStencilInfo{
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .stencilEnable = VK_FALSE
    };

    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    {
        vkw::PipelineInfo pipelineInfo{
            .rasterizationInfo = rasterInfo,
            .depthStencilInfo = depthStencilInfo,
            .pipelineLayout = depthPrePass.pipelineLayout,
            .renderPass = depthPrePass.renderPass.renderPass
        };
        depthPrePass.pipeline = rd->createPipeline(pipelineInfo, depthPrePass.shader);
    }

    {
        depthStencilInfo.depthWriteEnable = VK_FALSE;
        depthStencilInfo.compareOp = VK_COMPARE_OP_EQUAL;

        vkw::PipelineInfo pipelineInfo{
            .rasterizationInfo = rasterInfo,
            .depthStencilInfo = depthStencilInfo,
            .pipelineLayout = forwardPass.pipelineLayout,
            .renderPass = forwardPass.renderPass.renderPass
        };
        forwardPass.pipeline = rd->createPipeline(pipelineInfo, forwardPass.shader);
    }
}

void RenderSystem::createSyncObjects() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    VkDevice device = rd->getDevice();

    presentCompleteSemaphores.resize(maxFrameLag);
    renderCompleteSemaphores.resize(maxFrameLag);
    inFlightFences.resize(maxFrameLag);

    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr
    };

    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < maxFrameLag; i++) {
        CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphores[i]));

        CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphores[i]));

        CHECK_VKRESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]));
    }
}

void RenderSystem::update(Scene& scene) {
    // update uniform data ...
    scene.updateSceneBufferData();

    updateDescriptorSets(scene);
}

void RenderSystem::render() {
    // TODO: implement
}

void RenderSystem::updateDescriptorSets(Scene& scene) {
    // TODO: write descriptor sets

    // material descriptor created here?
    scene.updateSceneDescriptors(forwardPass.shader.layouts[1]);

    vkw::DescriptorWriter writer;

    for (uint32_t i = 0; i < maxFrameLag; i++) {
        writer.bindBuffer(0, &sceneDataBuffers[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.writeSet(forwardPass.descriptors[0 + i]);
    }
}

void RenderSystem::updateRenderSurfaces() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    rd->deviceWaitIdle();
    cleanupRenderSurfaces();

    rd->updateSwapchain(&width, &height);
    vkw::Swapchain swapChain = rd->getSwapChain();

    const auto color = rd->createTexture(vkw::TEXTURE_2D, { width, height },
            { swapChain.getFormat(),
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_FILTER_LINEAR,
                    VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    rd->getMSAASamples(),
                    false },
            0);

    vkw::AttachmentInfo attachment{ color };
    forwardPass.renderTarget.addColorAttachment(attachment);

    const auto depth = rd->createTexture(vkw::TEXTURE_DEPTH, { width, height },
            { .samples = rd->getMSAASamples() }, 0);

    attachment = vkw::AttachmentInfo{ depth };
    attachment.loadAction = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeAction = VK_ATTACHMENT_STORE_OP_STORE;
    forwardPass.renderTarget.setDepthStencilAttachment(attachment);

    attachment = vkw::AttachmentInfo{ depth };
    depthPrePass.renderTarget.setDepthStencilAttachment(attachment);

    attachment = vkw::AttachmentInfo{ &swapChain }; // TODO: potential problem? pointer goes out of scope after function
    attachment.loadAction = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    forwardPass.renderTarget.addColorResolveAttachment(attachment);

    forwardPass.setupRenderPass();
    depthPrePass.setupRenderPass();

    const uint32_t imageCount = swapChain.getImageCount();
    forwardPass.renderTarget.setupFramebuffers(imageCount, { width, height }, forwardPass.renderPass.renderPass);
    depthPrePass.renderTarget.setupFramebuffers(1, { width, height }, depthPrePass.renderPass.renderPass);

    Engine::getSingleton()->activeScene.getCamera()->updateViewportSize(width, height);
}

void RenderSystem::cleanupRenderSurfaces() {
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
    forwardPass.renderTarget.destroy();
    rd->destroyRenderPass(forwardPass.renderPass.name);

    depthPrePass.renderTarget.destroy();
    rd->destroyRenderPass(depthPrePass.renderPass.name);
}

} //namespace sublimation