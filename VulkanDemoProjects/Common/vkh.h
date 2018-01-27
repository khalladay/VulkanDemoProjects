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