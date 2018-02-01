#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include "debug.h"

#include "vkh_types.h"

namespace vkh
{
	void createDescriptorPool(VkDescriptorPool& outPool, const VkDevice& device, std::vector<VkDescriptorType>& descriptorTypes, std::vector<uint32_t>& maxDescriptors)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(descriptorTypes.size());
		uint32_t summedDescCount = 0;

		for (uint32_t i = 0; i < descriptorTypes.size(); ++i)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = descriptorTypes[i];
			poolSize.descriptorCount = maxDescriptors[i];
			poolSizes.push_back(poolSize);

			summedDescCount += poolSize.descriptorCount;
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = &poolSizes[0];
		poolInfo.maxSets = summedDescCount;

		VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &outPool);
		checkf(res == VK_SUCCESS, "Error creating descriptor pool");
	}

	void createImageView(VkImageView& outView, VkFormat imageFormat, VkImageAspectFlags aspectMask, uint32_t mipCount, const VkImage& imageHdl, const VkDevice& device)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = imageHdl;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = imageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = aspectMask;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipCount;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;


		VkResult res = vkCreateImageView(device, &createInfo, nullptr, &outView);
		checkf(res == VK_SUCCESS, "Error creating Image View");

	}

	void createVkSemaphore(VkSemaphore& outSemaphore, const VkDevice& device)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkResult res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &outSemaphore);
		checkf(res == VK_SUCCESS, "Error creating vk semaphore");
	}

	void createFence(VkFence& outFence, VkDevice& device)
	{
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = NULL;
		fenceInfo.flags = 0;
		VkResult vk_res = vkCreateFence(device, &fenceInfo, NULL, &outFence);
		checkf(vk_res == VK_SUCCESS, "Error creating vk fence");
	}

	void waitForFence(VkFence& fence, const VkDevice& device)
	{
		if (fence)
		{
			if (vkGetFenceStatus(device, fence) == VK_SUCCESS)
			{
				vkWaitForFences(device, 1, &fence, true, 0);
			}
		}
	}

	void createCommandPool(VkCommandPool& outPool, const VkDevice& lDevice, const VkhPhysicalDevice& physDevice, uint32_t queueFamilyIdx)
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIdx;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		VkResult res = vkCreateCommandPool(lDevice, &poolInfo, nullptr, &outPool);
		checkf(res == VK_SUCCESS, "Error creating command pool");
	}

	void freeDeviceMemory(Allocation& mem)
	{
		//this is sorta weird
		mem.context->allocator.free(mem);
	}

	void createRenderPass(VkRenderPass& outPass, std::vector<VkAttachmentDescription>& colorAttachments, VkAttachmentDescription* depthAttachment, const VkDevice& device)
	{
		std::vector<VkAttachmentReference> attachRefs;

		std::vector<VkAttachmentDescription> allAttachments;
		allAttachments = colorAttachments;

		uint32_t attachIdx = 0;
		while (attachIdx < colorAttachments.size())
		{
			VkAttachmentReference ref = { 0 };
			ref.attachment = attachIdx++;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachRefs.push_back(ref);
		}

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		subpass.pColorAttachments = &attachRefs[0];

		VkAttachmentReference depthRef = { 0 };

		if (depthAttachment)
		{
			VkAttachmentReference depthRef = { 0 };
			depthRef.attachment = attachIdx;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthRef;
			allAttachments.push_back(*depthAttachment);
		}


		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
		renderPassInfo.pAttachments = &allAttachments[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		//we need a subpass dependency for transitioning the image to the right format, because by default, vulkan
		//will try to do that before we have acquired an image from our fb
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL; //External means outside of the render pipeline, in srcPass, it means before the render pipeline
		dependency.dstSubpass = 0; //must be higher than srcSubpass
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		//add the dependency to the renderpassinfo
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &outPass);
		checkf(res == VK_SUCCESS, "Error creating render pass");
	}

	void createCommandBuffer(VkCommandBuffer& outBuffer, VkCommandPool& pool, const VkDevice& lDevice)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult res = vkAllocateCommandBuffers(lDevice, &allocInfo, &outBuffer);
		checkf(res == VK_SUCCESS, "Error creating command buffer");
	}

	uint32_t getMemoryType(const VkPhysicalDevice& device, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

		//The VkPhysicalDeviceMemoryProperties structure has two arraysL memoryTypes and memoryHeaps.
		//Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM 
		//for when VRAM runs out.The different types of memory exist within these heaps.Right now 
		//we'll only concern ourselves with the type of memory and not the heap it comes from, 
		//but you can imagine that this can affect performance.

		for (uint32_t memoryIndex = 0; memoryIndex < memProperties.memoryTypeCount; memoryIndex++)
		{
			const uint32_t memoryTypeBits = (1 << memoryIndex);
			const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

			const VkMemoryPropertyFlags properties = memProperties.memoryTypes[memoryIndex].propertyFlags;
			const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

			if (isRequiredMemoryType && hasRequiredProperties)
			{
				return static_cast<int32_t>(memoryIndex);
			}

		}

		checkf(0, "Could not find a valid memory type for requiredProperties");
		return 0;
	}

	void createFrameBuffers(std::vector<VkFramebuffer>& outBuffers, const VkhSwapChain& swapChain, const VkImageView* depthBufferView, const VkRenderPass& renderPass, const VkDevice& device)
	{
		outBuffers.resize(swapChain.imageViews.size());

		for (uint32_t i = 0; i < outBuffers.size(); i++)
		{
			std::vector<VkImageView> attachments;
			attachments.push_back(swapChain.imageViews[i]);

			if (depthBufferView)
			{
				attachments.push_back(*depthBufferView);
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = &attachments[0];
			framebufferInfo.width = swapChain.extent.width;
			framebufferInfo.height = swapChain.extent.height;
			framebufferInfo.layers = 1;

			VkResult r = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &outBuffers[i]);
			checkf(r == VK_SUCCESS, "Error creating framebuffer");
		}
	}

	void allocateDeviceMemory(Allocation& outMem, AllocationCreateInfo info, VkhContext& ctxt)
	{
		ctxt.allocator.alloc(outMem, info);
	}

	VkhCommandBuffer beginScratchCommandBuffer(ECommandPoolType type, VkhContext& ctxt)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (type == ECommandPoolType::Graphics)
		{
			allocInfo.commandPool = ctxt.gfxCommandPool;
		}
		else if (type == ECommandPoolType::Transfer)
		{
			allocInfo.commandPool = ctxt.transferCommandPool;
		}
		else
		{
			allocInfo.commandPool = ctxt.presentCommandPool;
		}

		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(ctxt.device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkhCommandBuffer outBuf;
		outBuf.buffer = commandBuffer;
		outBuf.owningPool = type;
		outBuf.context = &ctxt;

		return outBuf;

	}

	void submitScratchCommandBuffer(VkhCommandBuffer& commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer.buffer);

		checkf(commandBuffer.context, "Attempting to submit a scratch command buffer that does not have a valid context");
		VkhContext& ctxt = *commandBuffer.context;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer.buffer;

		VkQueue queue;
		VkCommandPool pool;
		if (commandBuffer.owningPool == ECommandPoolType::Graphics)
		{
			queue = ctxt.deviceQueues.graphicsQueue;
			pool = ctxt.gfxCommandPool;

		}
		else if (commandBuffer.owningPool == ECommandPoolType::Transfer)
		{
			queue = ctxt.deviceQueues.transferQueue;
			pool = ctxt.transferCommandPool;
		}
		else
		{
			queue = ctxt.deviceQueues.presentQueue;
			pool = ctxt.presentCommandPool;
		}


		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(ctxt.device, pool, 1, &commandBuffer.buffer);
	}


	void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset, VkhCommandBuffer& buffer)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = srcOffset; // Optional
		copyRegion.dstOffset = dstOffset; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(buffer.buffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset, VkhContext& ctxt)
	{
		VkhCommandBuffer scratch = beginScratchCommandBuffer(ECommandPoolType::Transfer, ctxt);

		copyBuffer(srcBuffer, dstBuffer, size, srcOffset, dstOffset, scratch);

		submitScratchCommandBuffer(scratch);
	}

	void createShaderModule(VkShaderModule& outModule, const char* binaryData, size_t dataSize, const VkhContext& ctxt)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		//data for vulkan is stored in uint32_t  -  so we have to temporarily copy it to a container that respects that alignment

		std::vector<uint32_t> codeAligned;
		codeAligned.resize((uint32_t)(dataSize / sizeof(uint32_t) + 1));

		memcpy(&codeAligned[0], binaryData, dataSize);
		createInfo.pCode = &codeAligned[0];
		createInfo.codeSize = dataSize;

		checkf(dataSize % 4 == 0, "Invalid data size for .spv file -> are you sure that it compiled correctly?");

		VkResult res = vkCreateShaderModule(ctxt.device, &createInfo, nullptr, &outModule);
		checkf(res == VK_SUCCESS, "Error creating shader module");

	}

	void createBuffer(VkBuffer& outBuffer, Allocation& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkhContext& ctxt)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		//concurrent so it can be used by the graphics and transfer queues

		std::vector<uint32_t> queues;
		queues.push_back(ctxt.gpu.graphicsQueueFamilyIdx);

		if (ctxt.gpu.graphicsQueueFamilyIdx != ctxt.gpu.transferQueueFamilyIdx)
		{
			queues.push_back(ctxt.gpu.transferQueueFamilyIdx);
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


		bufferInfo.pQueueFamilyIndices = &queues[0];
		bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

		VkResult res = vkCreateBuffer(ctxt.device, &bufferInfo, nullptr, &outBuffer);
		checkf(res == VK_SUCCESS, "Error creating buffer");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(ctxt.device, outBuffer, &memRequirements);

		AllocationCreateInfo allocInfo = {};
		allocInfo.size = memRequirements.size;
		allocInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
		allocInfo.usage = properties;

		ctxt.allocator.alloc(bufferMemory, allocInfo);
		vkBindBufferMemory(ctxt.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
	}

	void copyDataToBuffer(VkBuffer* buffer, uint32_t dataSize, uint32_t dstOffset, char* data, VkhContext& ctxt)
	{
		VkBuffer stagingBuffer;
		vkh::Allocation stagingMemory;

		vkh::createBuffer(stagingBuffer,
			stagingMemory,
			dataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ctxt);

		void* mappedStagingBuffer;
		vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset, dataSize, 0, &mappedStagingBuffer);

		memset(mappedStagingBuffer, 0, dataSize);
		memcpy(mappedStagingBuffer, data, dataSize);

		vkUnmapMemory(ctxt.device, stagingMemory.handle);

		vkh::VkhCommandBuffer scratch = vkh::beginScratchCommandBuffer(vkh::ECommandPoolType::Transfer, ctxt);
		vkh::copyBuffer(stagingBuffer, *buffer, dataSize, 0, dstOffset, scratch);
		vkh::submitScratchCommandBuffer(scratch);
		vkh::freeDeviceMemory(stagingMemory);
	}

	void createImage(VkImage& outImage, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, const VkhContext& ctxt)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult res = vkCreateImage(ctxt.device, &imageInfo, nullptr, &outImage);
		checkf(res == VK_SUCCESS, "Error creating vk image");
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkhContext& ctxt)
	{
		VkhCommandBuffer commandBuffer = beginScratchCommandBuffer(ECommandPoolType::Transfer, ctxt);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };


		vkCmdCopyBufferToImage(
			commandBuffer.buffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		submitScratchCommandBuffer(commandBuffer);


	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkhContext& ctxt)
	{
		VkhCommandBuffer commandBuffer = beginScratchCommandBuffer(ECommandPoolType::Graphics, ctxt);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		//these are used to transfer queue ownership, which we aren't doing
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			//Unsupported layout transition
			checkf(0, "Attempting an unsupported image layout transition");
		}

		vkCmdPipelineBarrier(
			commandBuffer.buffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);


		submitScratchCommandBuffer(commandBuffer);

	}

	void allocMemoryForImage(Allocation& outMem, const VkImage& image, VkMemoryPropertyFlags properties, VkhContext& ctxt)
	{
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(ctxt.device, image, &memRequirements);

		AllocationCreateInfo createInfo;
		createInfo.size = memRequirements.size;
		createInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
		createInfo.usage = properties;
		allocateDeviceMemory(outMem, createInfo, ctxt);
	}


	void createBuffer(VkBuffer& outBuffer, Allocation& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const VkhContext& ctxt)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		//concurrent so it can be used by the graphics and transfer queues

		std::vector<uint32_t> queues;
		queues.push_back(ctxt.gpu.graphicsQueueFamilyIdx);

		if (ctxt.gpu.graphicsQueueFamilyIdx != ctxt.gpu.transferQueueFamilyIdx)
		{
			queues.push_back(ctxt.gpu.transferQueueFamilyIdx);
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


		bufferInfo.pQueueFamilyIndices = &queues[0];
		bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

		VkResult res = vkCreateBuffer(ctxt.device, &bufferInfo, nullptr, &outBuffer);
		checkf(res == VK_SUCCESS, "Error creating Buffer");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(ctxt.device, outBuffer, &memRequirements);

		AllocationCreateInfo allocInfo = {};
		allocInfo.size = memRequirements.size;
		allocInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
		allocInfo.usage = properties;

		ctxt.allocator.alloc(bufferMemory, allocInfo);
		vkBindBufferMemory(ctxt.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
	}
}