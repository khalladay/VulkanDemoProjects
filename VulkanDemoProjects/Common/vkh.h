#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include "debug.h"

#include "vkh_types.h"
namespace vkh
{
	const uint32_t INVALID_QUEUE_FAMILY_IDX = -1;

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

	void createInstance(VkhContext& ctxt, const char* appName)
	{
		VkApplicationInfo app_info;

		//stype and pnext are the same usage as below
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = NULL;
		app_info.pApplicationName = appName;
		app_info.applicationVersion = 1;
		app_info.pEngineName = appName;
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
		res = vkCreateInstance(&inst_info, NULL, &ctxt.instance);

		checkf(res != VK_ERROR_INCOMPATIBLE_DRIVER, "Cannot create VkInstance - driver incompatible");
		checkf(res == VK_SUCCESS, "Cannot create VkInstance - unknown error");


	}

	void createWin32Surface(VkhContext& ctxt, HINSTANCE win32Instance, HWND wndHdl)
	{
		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = wndHdl;
		createInfo.hinstance = win32Instance;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		VkResult res = vkCreateWin32SurfaceKHR(ctxt.instance, &createInfo, nullptr, &ctxt.surface.surface);
		checkf(res == VK_SUCCESS, "Error creating win32 vulkan surface");
	}

	void createDebugCallback(VkhContext& ctxt)
	{
#if _DEBUG
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
		CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(ctxt.instance, "vkCreateDebugReportCallbackEXT");
		CreateDebugReportCallback(ctxt.instance, &createInfo, NULL, &callback);
#endif
	}

	void createPhysicalDevice(VkhContext& ctxt)
	{
		VkPhysicalDevice& outDevice = ctxt.gpu.device;

		uint32_t gpu_count;
		VkResult res = vkEnumeratePhysicalDevices(ctxt.instance, &gpu_count, NULL);

		// Allocate space and get the list of devices.
		std::vector<VkPhysicalDevice> gpus;
		gpus.resize(gpu_count);

		res = vkEnumeratePhysicalDevices(ctxt.instance, &gpu_count, gpus.data());
		checkf(res == VK_SUCCESS, "Could not enumerate physical devices");

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
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, ctxt.surface.surface, &scSupport.capabilities);

				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, ctxt.surface.surface, &formatCount, nullptr);

				if (formatCount != 0)
				{
					scSupport.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, ctxt.surface.surface, &formatCount, scSupport.formats.data());
				}
				else
				{
					continue;
				}

				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, ctxt.surface.surface, &presentModeCount, nullptr);

				if (presentModeCount != 0)
				{
					scSupport.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, ctxt.surface.surface, &presentModeCount, scSupport.presentModes.data());
				}

				bool worksWithSurface = scSupport.formats.size() > 0 && scSupport.presentModes.size() > 0;

				if (score > curScore && supportsAllRequiredExtensions && worksWithSurface)
				{
					found = true;

					ctxt.gpu.device = gpu;
					ctxt.gpu.swapChainSupport = scSupport;
					curScore = score;
				}
			}
		}

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(ctxt.gpu.device, &deviceProperties);
		printf("Selected GPU: %s\n", deviceProperties.deviceName);

		checkf(found, "Could not find a gpu that matches our specifications");

		vkGetPhysicalDeviceFeatures(outDevice, &ctxt.gpu.features);
		vkGetPhysicalDeviceMemoryProperties(outDevice, &ctxt.gpu.memProps);
		vkGetPhysicalDeviceProperties(outDevice, &ctxt.gpu.deviceProps);
		printf("Max mem allocations: %i\n", ctxt.gpu.deviceProps.limits.maxMemoryAllocationCount);

		//get queue families while we're here
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies;
		queueFamilies.resize(ctxt.gpu.queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, queueFamilies.data());

		std::vector<VkQueueFamilyProperties> queueVec;
		queueVec.resize(ctxt.gpu.queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, &queueVec[0]);


		// Iterate over each queue to learn whether it supports presenting:
		VkBool32 *pSupportsPresent = (VkBool32 *)malloc(ctxt.gpu.queueFamilyCount * sizeof(VkBool32));
		for (uint32_t i = 0; i < ctxt.gpu.queueFamilyCount; i++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(outDevice, i, ctxt.surface.surface, &pSupportsPresent[i]);
		}


		ctxt.gpu.graphicsQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
		ctxt.gpu.transferQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
		ctxt.gpu.presentQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;

		bool foundGfx = false;
		bool foundTransfer = false;
		bool foundPresent = false;

		for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); ++queueIdx)
		{
			const auto& queueFamily = queueFamilies[queueIdx];
			const auto& queueFamily2 = queueVec[queueIdx];

			if (!foundGfx && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				ctxt.gpu.graphicsQueueFamilyIdx = queueIdx;
				foundGfx = true;
			}

			if (!foundTransfer && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				ctxt.gpu.transferQueueFamilyIdx = queueIdx;
				foundTransfer = true;

			}

			if (!foundPresent && queueFamily.queueCount > 0 && pSupportsPresent[queueIdx])
			{
				ctxt.gpu.presentQueueFamilyIdx = queueIdx;
				foundPresent = true;

			}

			if (foundGfx && foundTransfer && foundPresent) break;
		}

		checkf(foundGfx && foundPresent && foundTransfer, "Failed to find all required device queues");
	}

	void createLogicalDevice(VkhContext& ctxt)
	{
		const VkhPhysicalDevice& physDevice = ctxt.gpu;
		VkDevice& outDevice = ctxt.device;
		VkhDeviceQueues& outQueues = ctxt.deviceQueues;

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
		checkf(res == VK_SUCCESS, "Error creating logical device");

		vkGetDeviceQueue(outDevice, physDevice.graphicsQueueFamilyIdx, 0, &ctxt.deviceQueues.graphicsQueue);
		vkGetDeviceQueue(outDevice, physDevice.transferQueueFamilyIdx, 0, &ctxt.deviceQueues.transferQueue);
		vkGetDeviceQueue(outDevice, physDevice.presentQueueFamilyIdx, 0, &ctxt.deviceQueues.presentQueue);

	}


}