#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#define VMA_IMPLEMENTATION
#include <gpuopen-allocator/vk_mem_alloc.h>

#include <vector>

namespace vkh
{
	const uint32_t INVALID_QUEUE_FAMILY_IDX = -1;

	enum ECommandPoolType
	{
		Graphics,
		Transfer,
		Present
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

	struct VkhLogicalDevice
	{
		VkDevice	device;
		VkQueue		graphicsQueue;
		VkQueue		transferQueue;
		VkQueue		presentQueue;
	};

	struct VkhImage
	{
		VkImage image;
		VmaAllocation allocation;
	};

	struct VkhRenderBuffer
	{
		VkImage			handle;
		VkImageView		view;
		VmaAllocation	imageMemory;
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
		VkCommandPool			gfxCommandPool;
		VkCommandPool			transferCommandPool;
		VkCommandPool			presentCommandPool;
		std::vector<VkFence>	frameFences;
		VkSemaphore				imageAvailableSemaphore;
		VkSemaphore				renderFinishedSemaphore;
		VkDescriptorPool		descriptorPool;

		VkFramebuffer*			framebuffers;
		uint32_t				numFramebuffers;
		vkh::VkhRenderBuffer	depthBuffer;

		VkRenderPass			mainRenderPass;
		VmaAllocator			allocator;
	};

	//init will set up this context variable, all other calls will use it. 
	//every call could technically be rewritten to support an arbitrary Context, but
	//I can't think of a time in the near future when I'd want that.
	VkhContext Context;

	void init(uint32_t width, uint32_t height, HINSTANCE Instance, HWND wndHdl, const char* applicationName);
	void createImage(VkImage& outImage, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);

}
