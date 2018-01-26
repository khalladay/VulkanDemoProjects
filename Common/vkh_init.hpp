#pragma once

//vkh_init - functions that will only be called during the initialization of the VkhContext

#include "vkh.hpp"

namespace _vkh
{
	using namespace vkh;

	VkDebugReportCallbackEXT callback;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData)
	{
		printf("[VALIDATION LAYER] %s \n", msg);
		return VK_FALSE;
	}

	void createWin32Context(VkhContext& outContext, uint32_t width, uint32_t height, HINSTANCE Instance, HWND wndHdl, const char* applicationName);
	void createWindowsInstance(VkInstance& outInstance, const char* applicationName);
	void createWin32Surface(VkhSurface& outSurface, VkInstance& vkInstance, HINSTANCE win32Instance, HWND wndHdl);
	void getDiscretePhysicalDevice(VkhPhysicalDevice& outDevice, VkInstance& inInstance, const VkhSurface& surface);
	void createLogicalDevice(VkDevice& outDevice, VkhDeviceQueues& outqueues, const VkhPhysicalDevice& physDevice);
	void createSwapchainForSurface(VkhSwapChain& outSwapChain, VkhPhysicalDevice& physDevice, const VkDevice& lDevice, const VkhSurface& surface);
	void createCommandPool(VkCommandPool& outPool, const VkDevice& lDevice, const VkhPhysicalDevice& physDevice, uint32_t queueFamilyIdx);


	void createWindowsInstance(VkInstance& outInstance, const char* applicationName)
	{
		VkApplicationInfo app_info;

		//stype and pnext are the same usage as below
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = NULL;
		app_info.pApplicationName = applicationName;
		app_info.applicationVersion = 1;
		app_info.pEngineName = applicationName;
		app_info.engineVersion = 1;
		app_info.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> validationLayers;
		std::vector<bool> layersAvailable;

#if _DEBUG
		printf("Starting up with validation layers enabled: \n");

		const char* layerNames = "VK_LAYER_LUNARG_standard_validation";

		validationLayers.push_back(layerNames);
		printf("Looking for: %s\n", layerNames);

		layersAvailable.push_back(false);

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers;
		availableLayers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (uint32_t i = 0; i < validationLayers.size(); ++i)
		{
			for (uint32_t j = 0; j < availableLayers.size(); ++j)
			{
				if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0)
				{
					printf("Found layer: %s\n", validationLayers[i]);
					layersAvailable[i] = true;
				}
			}
		}

		bool foundAllLayers = true;
		for (uint32_t i = 0; i < layersAvailable.size(); ++i)
		{
			foundAllLayers &= layersAvailable[i];
		}

		checkf(foundAllLayers, "Not all required validation layers were found.");
#endif

		//Get available extensions:
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions;
		extensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::vector<const char*> requiredExtensions;
		std::vector<bool> extensionsPresent;

		requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		extensionsPresent.push_back(false);

		requiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		extensionsPresent.push_back(false);

#if _DEBUG
		requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		extensionsPresent.push_back(false);
#endif


		printf("Available Vulkan Extensions: \n");

		for (uint32_t i = 0; i < extensions.size(); ++i)
		{
			auto& prop = extensions[i];
			printf("* %s ", prop.extensionName);
			bool found = false;
			for (uint32_t i = 0; i < requiredExtensions.size(); i++)
			{
				if (strcmp(prop.extensionName, requiredExtensions[i]) == 0)
				{
					printf(" - Enabled\n");
					found = true;
					extensionsPresent[i] = true;
				}
			}

			if (!found) printf("\n");
		}

		bool allExtensionsFound = true;
		for (uint32_t i = 0; i < extensionsPresent.size(); i++)
		{
			allExtensionsFound &= extensionsPresent[i];
		}

		checkf(allExtensionsFound, "Failed to find all required vulkan extensions");

		//create instance with all extensions

		VkInstanceCreateInfo inst_info;

		//useful for driver validation and when passing as void*
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		//used to pass extension info when stype is extension defined
		inst_info.pNext = NULL;
		inst_info.flags = 0;
		inst_info.pApplicationInfo = &app_info;
		inst_info.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		inst_info.ppEnabledExtensionNames = requiredExtensions.data();

		//validation layers / other layers
		inst_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		inst_info.ppEnabledLayerNames = validationLayers.data();

		VkResult res;
		res = vkCreateInstance(&inst_info, NULL, &outInstance);

		checkf(res != VK_ERROR_INCOMPATIBLE_DRIVER, "Cannot create VkInstance - driver incompatible");
		checkf(res == VK_SUCCESS, "Cannot create VkInstance - unknown error");

#if _DEBUG
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
		CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(outInstance, "vkCreateDebugReportCallbackEXT");
		CreateDebugReportCallback(outInstance, &createInfo, NULL, &callback);
#endif
	}

	void createWin32Context(VkhContext& outContext, uint32_t width, uint32_t height, HINSTANCE Instance, HWND wndHdl, const char* applicationName)
	{
		createWindowsInstance(outContext.instance, applicationName);
		createWin32Surface(outContext.surface, outContext.instance, Instance, wndHdl);
		getDiscretePhysicalDevice(outContext.gpu, outContext.instance, outContext.surface);
		createLogicalDevice(outContext.device, outContext.deviceQueues, outContext.gpu);

		createSwapchainForSurface(outContext.swapChain, outContext.gpu, outContext.device, outContext.surface);
		createCommandPool(outContext.gfxCommandPool, outContext.device, outContext.gpu, outContext.gpu.graphicsQueueFamilyIdx);
		createCommandPool(outContext.transferCommandPool, outContext.device, outContext.gpu, outContext.gpu.transferQueueFamilyIdx);
		createCommandPool(outContext.presentCommandPool, outContext.device, outContext.gpu, outContext.gpu.presentQueueFamilyIdx);

		//since we're only creating one of these for each material, this means we'll support 128 materials

		std::vector<VkDescriptorType> types;
		types.reserve(3);
		types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);

		std::vector<uint32_t> counts;
		counts.reserve(3);
		counts.push_back(128);
		counts.push_back(128);
		counts.push_back(128);
		counts.push_back(8192);
		counts.push_back(128);

		createDescriptorPool(outContext.descriptorPool, outContext.device, types, counts);

		createVkSemaphore(outContext.imageAvailableSemaphore, outContext.device);
		createVkSemaphore(outContext.renderFinishedSemaphore, outContext.device);

		outContext.frameFences.resize(outContext.swapChain.imageViews.size());
		for (uint32_t i = 0; i < outContext.frameFences.size(); ++i)
		{
			createFence(outContext.frameFences[i], outContext.device);
		}
	}

	void createWin32Surface(VkhSurface& outSurface, VkInstance& vkInstance, HINSTANCE win32Instance, HWND wndHdl)
	{
		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = wndHdl;
		createInfo.hinstance = win32Instance;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		VkResult res = vkCreateWin32SurfaceKHR(vkInstance, &createInfo, nullptr, &outSurface.surface);
		checkf(res == VK_SUCCESS, "Error creating win32 surface");
	}

	void getDiscretePhysicalDevice(VkhPhysicalDevice& outDevice, VkInstance& inInstance, const VkhSurface& surface)
	{
		uint32_t gpu_count;
		VkResult res = vkEnumeratePhysicalDevices(inInstance, &gpu_count, NULL);

		// Allocate space and get the list of devices.
		std::vector<VkPhysicalDevice> gpus;
		gpus.resize(gpu_count);

		res = vkEnumeratePhysicalDevices(inInstance, &gpu_count, gpus.data());

		bool found = false;
		int curScore = 0;

		std::vector<const char*> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		for (uint32_t i = 0; i < gpus.size(); ++i)
		{
			const auto& gpu = gpus[i];

			auto props = VkPhysicalDeviceProperties();
			vkGetPhysicalDeviceProperties(gpu, &props);

			if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				VkPhysicalDeviceProperties deviceProperties;
				VkPhysicalDeviceFeatures deviceFeatures;
				vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
				vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);

				int score = 1000;
				score += props.limits.maxImageDimension2D;
				score += props.limits.maxFragmentInputComponents;
				score += deviceFeatures.geometryShader ? 1000 : 0;
				score += deviceFeatures.tessellationShader ? 1000 : 0;

				if (!deviceFeatures.shaderSampledImageArrayDynamicIndexing)
				{
					continue;
				}

				//make sure the device supports presenting

				uint32_t extensionCount;
				vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

				std::vector<VkExtensionProperties> availableExtensions;
				availableExtensions.resize(extensionCount);
				vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

				uint32_t foundExtensionCount = 0;
				for (uint32_t extIdx = 0; extIdx < availableExtensions.size(); ++extIdx)
				{
					for (uint32_t reqIdx = 0; reqIdx < deviceExtensions.size(); ++reqIdx)
					{
						if (strcmp(availableExtensions[extIdx].extensionName, deviceExtensions[i]) == 0)
						{
							foundExtensionCount++;
						}
					}
				}

				bool supportsAllRequiredExtensions = deviceExtensions.size() == foundExtensionCount;
				if (!supportsAllRequiredExtensions)
				{
					continue;
				}

				//make sure the device supports at least one valid image format for our surface
				VkhSwapChainSupportInfo scSupport;
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface.surface, &scSupport.capabilities);

				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface.surface, &formatCount, nullptr);

				if (formatCount != 0)
				{
					scSupport.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface.surface, &formatCount, scSupport.formats.data());
				}
				else
				{
					continue;
				}

				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface.surface, &presentModeCount, nullptr);

				if (presentModeCount != 0)
				{
					scSupport.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface.surface, &presentModeCount, scSupport.presentModes.data());
				}

				bool worksWithSurface = scSupport.formats.size() > 0 && scSupport.presentModes.size() > 0;

				if (score > curScore && supportsAllRequiredExtensions && worksWithSurface)
				{
					found = true;

					outDevice.device = gpu;
					outDevice.swapChainSupport = scSupport;
					curScore = score;
				}
			}
		}

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(outDevice.device, &deviceProperties);
		printf("Selected GPU: %s\n", deviceProperties.deviceName);

		checkf(found, "Could not find suitable device");
		checkf(res == VK_SUCCESS, "");

		vkGetPhysicalDeviceFeatures(outDevice.device, &outDevice.features);
		vkGetPhysicalDeviceMemoryProperties(outDevice.device, &outDevice.memProps);
		vkGetPhysicalDeviceProperties(outDevice.device, &outDevice.deviceProps);
		printf("Max mem allocations: %i\n", outDevice.deviceProps.limits.maxMemoryAllocationCount);

		//get queue families while we're here
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice.device, &outDevice.queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies;
		queueFamilies.resize(outDevice.queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice.device, &outDevice.queueFamilyCount, queueFamilies.data());

		std::vector<VkQueueFamilyProperties> queueVec;
		queueVec.resize(outDevice.queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice.device, &outDevice.queueFamilyCount, &queueVec[0]);


		// Iterate over each queue to learn whether it supports presenting:
		VkBool32 *pSupportsPresent = (VkBool32 *)malloc(outDevice.queueFamilyCount * sizeof(VkBool32));
		for (uint32_t i = 0; i < outDevice.queueFamilyCount; i++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(outDevice.device, i, surface.surface, &pSupportsPresent[i]);
		}


		outDevice.graphicsQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
		outDevice.transferQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
		outDevice.presentQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;

		bool foundGfx = false;
		bool foundTransfer = false;
		bool foundPresent = false;

		for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); ++queueIdx)
		{
			const auto& queueFamily = queueFamilies[queueIdx];
			const auto& queueFamily2 = queueVec[queueIdx];

			if (!foundGfx && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				outDevice.graphicsQueueFamilyIdx = queueIdx;
				foundGfx = true;
			}

			if (!foundTransfer && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				outDevice.transferQueueFamilyIdx = queueIdx;
				foundTransfer = true;

			}

			if (!foundPresent && queueFamily.queueCount > 0 && pSupportsPresent[queueIdx])
			{
				outDevice.presentQueueFamilyIdx = queueIdx;
				foundPresent = true;

			}

			if (foundGfx && foundTransfer && foundPresent) break;
		}

		checkf(foundGfx && foundPresent && foundTransfer, "Failed to find all required device queues");
	}

	void createLogicalDevice(VkDevice& outDevice, VkhDeviceQueues& outqueues, const VkhPhysicalDevice& physDevice)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::vector<uint32_t> uniqueQueueFamilies;

		uniqueQueueFamilies.push_back(physDevice.graphicsQueueFamilyIdx);
		if (physDevice.transferQueueFamilyIdx != physDevice.graphicsQueueFamilyIdx)
		{
			uniqueQueueFamilies.push_back(physDevice.transferQueueFamilyIdx);
		}

		if (physDevice.presentQueueFamilyIdx != physDevice.graphicsQueueFamilyIdx &&
			physDevice.presentQueueFamilyIdx != physDevice.transferQueueFamilyIdx)
		{
			uniqueQueueFamilies.push_back(physDevice.presentQueueFamilyIdx);
		}

		for (uint32_t familyIdx = 0; familyIdx < uniqueQueueFamilies.size(); ++familyIdx)
		{
			uint32_t queueFamily = uniqueQueueFamilies[familyIdx];

			VkDeviceQueueCreateInfo queueCreateInfo = {};

			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;

			float queuePriority[] = { 1.0f };
			queueCreateInfo.pQueuePriorities = queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);

		}

		//we don't need anything fancy right now, but this is where you require things
		// like geo shader support

		std::vector<const char*> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		std::vector<const char*> validationLayers;

#ifndef _DEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
		validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

		//don't do anything else here because we already know the validation layer is available, 
		//else we would have asserted earlier
#endif

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult res = vkCreateDevice(physDevice.device, &createInfo, nullptr, &outDevice);
		checkf(res == VK_SUCCESS, "Error creating device");

		vkGetDeviceQueue(outDevice, physDevice.graphicsQueueFamilyIdx, 0, &outqueues.graphicsQueue);
		vkGetDeviceQueue(outDevice, physDevice.transferQueueFamilyIdx, 0, &outqueues.transferQueue);
		vkGetDeviceQueue(outDevice, physDevice.presentQueueFamilyIdx, 0, &outqueues.presentQueue);

	}

	void createSwapchainForSurface(VkhSwapChain& outSwapChain, VkhPhysicalDevice& physDevice, const VkDevice& lDevice, const VkhSurface& surface)
	{
		//choose the surface format to use
		VkSurfaceFormatKHR desiredFormat;
		VkPresentModeKHR desiredPresentMode;
		VkExtent2D swapExtent;

		bool foundFormat = false;

		//if there is no preferred format, the formats array only contains VK_FORMAT_UNDEFINED
		if (physDevice.swapChainSupport.formats.size() == 1 && physDevice.swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
		{
			desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			desiredFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
			foundFormat = true;
		}

		//otherwise we can't just choose any format we want, but still let's try to grab one that we know will work for us first
		if (!foundFormat)
		{
			for (uint32_t i = 0; i < physDevice.swapChainSupport.formats.size(); ++i)
			{
				const auto& availableFormat = physDevice.swapChainSupport.formats[i];

				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					desiredFormat = availableFormat;
				}
			}
		}

		//if our preferred format isn't available, let's just grab the first available because yolo
		if (!foundFormat)
		{
			desiredFormat = physDevice.swapChainSupport.formats[0];
		}

		//present mode - VK_PRESENT_MODE_MAILBOX_KHR is for triple buffering, VK_PRESENT_MODE_FIFO_KHR is double, VK_PRESENT_MODE_IMMEDIATE_KHR is single
		//VK_PRESENT_MODE_FIFO_KHR  is guaranteed to be available.
		//let's prefer triple buffering, and fall back to double if it isn't supported

		desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < physDevice.swapChainSupport.presentModes.size(); ++i)
		{
			const auto& availablePresentMode = physDevice.swapChainSupport.presentModes[i];

			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				desiredPresentMode = availablePresentMode;
			}
		}

		//update physdevice for new surface size
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice.device, surface.surface, &physDevice.swapChainSupport.capabilities);

		//swap extent is the resolution of the swapchain
		swapExtent = physDevice.swapChainSupport.capabilities.currentExtent;

		//need 1 more than minimum image count for triple buffering
		uint32_t imageCount = physDevice.swapChainSupport.capabilities.minImageCount + 1;
		if (physDevice.swapChainSupport.capabilities.maxImageCount > 0 && imageCount > physDevice.swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = physDevice.swapChainSupport.capabilities.maxImageCount;
		}

		//now that everything is set up, we need to actually create the swap chain
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = desiredFormat.format;
		createInfo.imageColorSpace = desiredFormat.colorSpace;
		createInfo.imageExtent = swapExtent;
		createInfo.imageArrayLayers = 1; //always 1 unless a stereoscopic app

										 //here, we're rendering directly to the swap chain, but if we were using post processing, this might be VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


		if (physDevice.graphicsQueueFamilyIdx != physDevice.presentQueueFamilyIdx)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			uint32_t queueFamilyIndices[] = { physDevice.graphicsQueueFamilyIdx, physDevice.presentQueueFamilyIdx };
			createInfo.pQueueFamilyIndices = queueFamilyIndices;

		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional	
		}

		createInfo.preTransform = physDevice.swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = desiredPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.pNext = NULL;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult res = vkCreateSwapchainKHR(lDevice, &createInfo, nullptr, &outSwapChain.swapChain);
		checkf(res == VK_SUCCESS, "Error creating swapchain");

		//get images for swap chain
		vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, nullptr);
		outSwapChain.imageHandles.resize(imageCount);
		outSwapChain.imageViews.resize(imageCount);

		vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, &outSwapChain.imageHandles[0]);

		outSwapChain.imageFormat = desiredFormat.format;
		outSwapChain.extent = swapExtent;

		//create image views
		for (uint32_t i = 0; i < outSwapChain.imageHandles.size(); i++)
		{
			createImageView(outSwapChain.imageViews[i], outSwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, outSwapChain.imageHandles[i], lDevice);
		}
	}

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




}
