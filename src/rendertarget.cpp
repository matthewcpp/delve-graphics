#include "vkdev/rendertarget.h"

#include <array>
#include <memory>

namespace vkdev {

void SwapChainRenderTarget::create(SwapChain& swapchain_, CommandPool& commandPool) {
    // this will retrieve the format we will use to create the depth buffer image
    // note that we are requiring that the format support a stencil buffer component
    const VkFormat depthFormat = Image::findSupportedFormat(
        device,
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    swapchain = &swapchain_;

    createImages(commandPool, depthFormat);
    createRenderPass(depthFormat);
    createFramebuffers();
}

void SwapChainRenderTarget::createImages(CommandPool& commandPool, VkFormat depthFormat) {
    depthImage = std::make_unique<vkdev::Image>(device);
    depthImage->create(swapchain->extent.width, swapchain->extent.height, 1, msaaSampleCount, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    depthImage->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
    depthImage->transitionLayout(commandPool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);  // note this is optional in this case

    // create the multisampled color image buffer.  Note that multisampled images should not have multiple mip levels (enforced by the spec)
    // We are only ever rendering one image at a time, so only one multisampled image is needed
    msaaColorImage = std::make_unique<vkdev::Image>(device);
    msaaColorImage->create(swapchain->extent.width, swapchain->extent.height, 1, msaaSampleCount, swapchain->imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    msaaColorImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void SwapChainRenderTarget::createRenderPass(VkFormat depthFormat) {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain->imageFormat;
    colorAttachment.samples = msaaSampleCount; // multisampling

    // These apply to color and depth data
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // we will clear to black b4 rendering each frame
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthFormat; // this should be the same as the depth image itself
    depthAttachment.samples = msaaSampleCount;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear depth buffer before rendering
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // currently not used after drawing has finished so we dont care how its stored.
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // note that multisampled color attachments cannot be presented directly to the screen.  They need to be resolved to an regular image.
    // this is not required for depth attachments because they are not presented to the screen!
    // If MSAA is disabled then we should not create a resolve attachment.  Doing so will cause validation error
    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = swapchain->imageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT; // we need to convert the image to 1 sample per pixel
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // signals we need this in a format that can be presented to the screen via swapchain.

    std::array<VkAttachmentDescription, 3> attachmentDescriptions = { colorAttachment, depthAttachment, colorAttachmentResolve };

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentReference; // note that subpass can only have 1 depth + stencil attachment
    subpass.pResolveAttachments = &colorAttachmentResolveRef; // handles converting multisampled image to single sampled image for presentation

    // we need to configure the subpass of our renderpass to wait for the color attachment stage of the pipeline to begin
    // could also set waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;

    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;

    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(device.logical, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass.");
    }
}

void SwapChainRenderTarget::createFramebuffers() {
    framebuffers.resize(swapchain->imageViews.size());

    for (size_t i = 0; i < swapchain->imageViews.size(); i++) {
        // The color attachment differs for every swap chain image, but the same depth image can be used by all of them because only a single subpass is running at the same time due to our semaphores
        std::array<VkImageView, 3> attachments = { msaaColorImage->view, depthImage->view, swapchain->imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain->extent.width;
        framebufferInfo.height = swapchain->extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.logical, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer for swap chain images");
        }
    }
}

void SwapChainRenderTarget::cleanup() {
    depthImage->cleanup();
    msaaColorImage->cleanup();

    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device.logical, framebuffer, nullptr);
    }

    vkDestroyRenderPass(device.logical, renderPass, nullptr);
}

}