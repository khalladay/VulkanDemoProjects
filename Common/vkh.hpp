#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include <gpuopen-allocator\vk_mem_alloc.h>

#include "debug.hpp"
#include "vkh_init.hpp"

namespace vkh
{
	const uint32_t INVALID_QUEUE_FAMILY_IDX = -1;
	struct VkhContext;

	enum ECommandPoolType
	{
		Graphics,
		Transfer,
		Present
	};

	struct Allocation
	{
		VkDeviceMemory handle;
		uint32_t type;
		uint32_t id;
		VkDeviceSize size;
		VkDeviceSize offset;
	};

	struct AllocationCreateInfo
	{
		VkMemoryPropertyFlags usage;
		uint32_t memoryTypeIndex;
		VkDeviceSize size;
	};

	struct AllocatorInterface
	{
		void(*activate)(VkhContext*);
		void(*alloc)(Allocation&, AllocationCreateInfo);
		void(*free)(Allocation&);
		size_t(*allocatedSize)(uint32_t);
		uint32_t(*numAllocs)();
	};

	struct VkhSurface
	{
		VkSurfaceKHR surface;
		VkFormat format;
	};

	struct VkhCommandBuffer
	{
		VkCommandBuffer buffer;
		ECommandPoolType owningPool;
	};

	struct VkhSwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VkhSwapChain
	{
		VkSwapchainKHR				swapChain;
		VkFormat					imageFormat;
		VkExtent2D					extent;
		std::vector<VkImage>		imageHandles;
		std::vector<VkImageView>	imageViews;
	};

	struct VkhRenderBuffer
	{
		VkImage			handle;
		VkImageView		view;
		Allocation		imageMemory;
	};

	struct VkhLogicalDevice
	{
		VkDevice	device;
		VkQueue		graphicsQueue;
		VkQueue		transferQueue;
		VkQueue		presentQueue;
	};

	struct VkhPhysicalDevice
	{
		VkPhysicalDevice					device;
		VkPhysicalDeviceProperties			deviceProps;
		VkPhysicalDeviceMemoryProperties	memProps;
		VkPhysicalDeviceFeatures			features;
		VkhSwapChainSupportInfo				swapChainSupport;
		uint32_t							queueFamilyCount;
		uint32_t							presentQueueFamilyIdx;
		uint32_t							graphicsQueueFamilyIdx;
		uint32_t							transferQueueFamilyIdx;
	};

	struct VkhDeviceQueues
	{
		VkQueue		graphicsQueue;
		VkQueue		transferQueue;
		VkQueue		presentQueue;
	};

	struct VkhContext
	{
		VkInstance				instance;
		VkhSurface				surface;
		VkhPhysicalDevice		gpu;
		VkDevice				device;
		VkhDeviceQueues			deviceQueues;
		VkhSwapChain			swapChain;
		VkhRenderBuffer			depthBuffer;
		VkCommandPool			gfxCommandPool;
		VkCommandPool			transferCommandPool;
		VkCommandPool			presentCommandPool;
		std::vector<VkFence>	frameFences;
		VkSemaphore				imageAvailableSemaphore;
		VkSemaphore				renderFinishedSemaphore;
		
		VmaAllocator			allocator;
		VkDescriptorPool		descriptorPool;

		//hate this being here, but if material can create itself
		//this is where it has to live, otherwise rendering has to return
		//a vulkan type from a function and that means adding vkh.h to renderer.h
		VkRenderPass		mainRenderPass;
	};

	extern vkh::VkhContext GContext;

	struct VkhLoadStoreOp
	{
		VkAttachmentLoadOp load;
		VkAttachmentStoreOp store;
	};


	void createVkSemaphore(VkSemaphore& outSemaphore, const VkDevice& device);
	void createCommandBuffer(VkCommandBuffer& outBuffer, VkCommandPool& pool, const VkDevice& lDevice);
	void createRenderPass(VkRenderPass& outPass, std::vector<VkAttachmentDescription>& colorAttachments, VkAttachmentDescription* depthAttachment, const VkDevice& device);
	void createFrameBuffers(std::vector<VkFramebuffer>& outBuffers, const VkhSwapChain& swapChain, const VkImageView* depthBufferView, const VkRenderPass& renderPass, const VkDevice& device);
	uint32_t getMemoryType(const VkPhysicalDevice& device, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);

	void initContext()
	{
		_vkh::createWin32Context(GContext, GAppInfo.curW, GAppInfo.curH, GAppInfo.instance, GAppInfo.wndHdl, APP_NAME);
		_vkh::createDepthBuffer(depthBuffer, GAppInfo.curW, GAppInfo.curH, GContext.device, GContext.gpu.device);

		createMainRenderPass();

		vkh::createFrameBuffers(frameBuffers, GContext.swapChain, &depthBuffer.view, GContext.mainRenderPass, GContext.device);

		uint32_t swapChainImageCount = static_cast<uint32_t>(GContext.swapChain.imageViews.size());
		commandBuffers.resize(swapChainImageCount);
		for (uint32_t i = 0; i < swapChainImageCount; ++i)
		{
			vkh::createCommandBuffer(commandBuffers[i], GContext.gfxCommandPool, GContext.device);
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

	void createVkSemaphore(VkSemaphore& outSemaphore, const VkDevice& device)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkResult res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &outSemaphore);
		checkf(res == VK_SUCCESS, "Error creating semaphore");
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

	void createDepthBuffer(VkhRenderBuffer& outBuffer, uint32_t width, uint32_t height, const VkDevice& device, const VkPhysicalDevice& gpu)
	{
		createImage(outBuffer.handle,
			width,
			height,
			depthFormat(),
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			device);

		allocBindImageToMem(outBuffer.imageMemory, outBuffer.handle, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, gpu);

		createImageView(outBuffer.view, depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1, outBuffer.handle, device);
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

		assert(0);
		return 0;
	}
}