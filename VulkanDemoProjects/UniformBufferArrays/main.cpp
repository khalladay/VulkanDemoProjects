#include "Common.h"
#include <glm\glm.hpp>

#define BUFFER_ARRAY_SIZE 8
#define SHARED_UNIFORM_SIZE 48

vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMeshes[4];

	std::vector<VkFramebuffer>		frameBuffers;
	std::vector<VkCommandBuffer>	commandBuffers;

	VkRenderPass					mainRenderPass;

	VkDescriptorSet					descriptorSet;
	VkDescriptorSetLayout			descSetLayout;

	VkPipelineLayout				pipelineLayout[2];
	VkPipeline						graphicsPipeline[2];

	VkBuffer						sharedBuffer;
	vkh::Allocation					bufferMemory;
};

DemoData demoData;

void setupDemo();
void createMainRenderPass();
void setupDescriptorSet();
void writeDescriptorSet();
void mainLoop();
void shutdown();
void logFPSAverage(double avg);
void render();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE pInstance, LPSTR cmdLine, int showCode)
{
	HWND wndHdl = OS::makeWindow(Instance, "Uniform Buffer Array Demo", 800, 600);
	OS::initializeInput();

	vkh::VkhContextCreateInfo ctxtInfo = {};
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	ctxtInfo.typeCounts.push_back(32);

	initContext(ctxtInfo, "Uniform Buffer Array Demo", Instance, wndHdl, appContext);
	setupDemo();
	mainLoop();
	shutdown();

	return 0;
}

void setupDemo()
{
	vkh::Mesh::quad(demoData.quadMeshes[0], appContext, 1.0f, 1.0f, -0.5f, 0.5f);
	vkh::Mesh::quad(demoData.quadMeshes[1], appContext, 1.0f, 1.0f, 0.5f, 0.5f);
	vkh::Mesh::quad(demoData.quadMeshes[2], appContext, 1.0f, 1.0f, -0.5f, -0.5f);
	vkh::Mesh::quad(demoData.quadMeshes[3], appContext, 1.0f, 1.0f, 0.5f, -0.5f);

	createMainRenderPass();

	vkh::createFrameBuffers(demoData.frameBuffers, appContext.swapChain, nullptr, demoData.mainRenderPass, appContext.device);

	uint32_t swapChainImageCount = static_cast<uint32_t>(appContext.swapChain.imageViews.size());
	demoData.commandBuffers.resize(swapChainImageCount);
	for (uint32_t i = 0; i < swapChainImageCount; ++i)
	{
		vkh::createCommandBuffer(demoData.commandBuffers[i], appContext.gfxCommandPool, appContext.device);
	}

	setupDescriptorSet();

	vkh::VkhMaterialCreateInfo createInfo = {};
	createInfo.renderPass = demoData.mainRenderPass;
	createInfo.outPipeline = &demoData.graphicsPipeline[0];
	createInfo.outPipelineLayout = &demoData.pipelineLayout[0];
	createInfo.descSetLayouts.push_back(demoData.descSetLayout);

	vkh::createBasicMaterial("shaders\\common_vert.spv", "shaders\\frag1.spv", appContext, createInfo);

	vkh::VkhMaterialCreateInfo createInfo2 = {};
	createInfo2.renderPass = demoData.mainRenderPass;
	createInfo2.outPipeline = &demoData.graphicsPipeline[1];
	createInfo2.outPipelineLayout = &demoData.pipelineLayout[1];
	createInfo2.descSetLayouts.push_back(demoData.descSetLayout);

	vkh::createBasicMaterial("shaders\\common_vert.spv", "shaders\\frag2.spv", appContext, createInfo2);

	writeDescriptorSet();
}


void setupDescriptorSet()
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding = vkh::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

	VkDescriptorSetLayoutCreateInfo layoutInfo = vkh::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
	VkResult res = vkCreateDescriptorSetLayout(appContext.device, &layoutInfo, nullptr, &demoData.descSetLayout);
	checkf(res == VK_SUCCESS, "Error creating desc set layout");

	VkDescriptorSetAllocateInfo allocInfo = vkh::descriptorSetAllocateInfo(&demoData.descSetLayout, 1, appContext.descriptorPool);
	res = vkAllocateDescriptorSets(appContext.device, &allocInfo, &demoData.descriptorSet);
	checkf(res == VK_SUCCESS, "Error allocating global descriptor set");
}

void writeDescriptorSet()
{
	__declspec(align(16)) struct LayoutA
	{
		__declspec(align(16)) glm::vec4 colorA;
		__declspec(align(16)) glm::vec4 colorB;
		__declspec(align(16)) glm::vec4 colorC;

	};

	__declspec(align(16)) struct LayoutB
	{
		__declspec(align(16)) float r;
		__declspec(align(16)) glm::vec4 colorA;
		__declspec(align(16)) int x;
	};

	static_assert(sizeof(LayoutA) == sizeof(LayoutB), "Both shader uniform layouts must be the same size");
	static_assert(sizeof(LayoutA) == SHARED_UNIFORM_SIZE, "LayoutA is an unexpected size");
	
	char* sharedData = (char*)malloc(sizeof(LayoutA) * BUFFER_ARRAY_SIZE);
	LayoutA first = { glm::vec4(0.5,0,0,0), glm::vec4(0.25,0.5,0,0), glm::vec4(0.0,0.25,0.25,1) };
	LayoutB second = { 1.0, glm::vec4(1,1,1,1), 1};
	LayoutA third = { glm::vec4(0.0,0,0,0), glm::vec4(0.0,0.75,0,0), glm::vec4(0.0,0.25,0.25,1) };
	LayoutB fourth = { 0.0, glm::vec4(0,0,1,1), 1 };

	char* writeLocation = sharedData;
	memcpy(writeLocation, &first, SHARED_UNIFORM_SIZE);
	memcpy((writeLocation += SHARED_UNIFORM_SIZE), &second, SHARED_UNIFORM_SIZE);
	memcpy((writeLocation += SHARED_UNIFORM_SIZE), &third, SHARED_UNIFORM_SIZE);
	memcpy((writeLocation += SHARED_UNIFORM_SIZE), &fourth, SHARED_UNIFORM_SIZE);

	vkh::createBuffer(demoData.sharedBuffer,
		demoData.bufferMemory,
		SHARED_UNIFORM_SIZE * BUFFER_ARRAY_SIZE,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		appContext);


	vkh::copyDataToBuffer(&demoData.sharedBuffer, SHARED_UNIFORM_SIZE * BUFFER_ARRAY_SIZE, 0, sharedData, appContext);

	VkWriteDescriptorSet setWrite;

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = demoData.sharedBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = 0;
	setWrite.dstArrayElement = 0;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setWrite.descriptorCount = 1;
	setWrite.dstSet = demoData.descriptorSet;
	setWrite.pBufferInfo = &bufferInfo;
	setWrite.pImageInfo = 0;

	vkUpdateDescriptorSets(appContext.device, 1, &setWrite, 0, nullptr);

}

void createMainRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = appContext.swapChain.imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	std::vector<VkAttachmentDescription> renderPassAttachments;
	renderPassAttachments.push_back(colorAttachment);

	vkh::createRenderPass(demoData.mainRenderPass, renderPassAttachments, nullptr, appContext.device);

}


void logFPSAverage(double avg)
{
	printf("AVG FRAMETIME FOR LAST %i FRAMES: %f ms\n", FPS_DATA_FRAME_HISTORY_SIZE, avg);
}

void mainLoop()
{
	bool running = true;

	FPSData fpsData = { 0 };

	//fpsData.logCallback = logFPSAverage;

	startTimingFrame(fpsData);

	while (running)
	{
		double dt = endTimingFrame(fpsData);
		startTimingFrame(fpsData);

		OS::handleEvents();
		OS::pollInput();

		if (OS::getKey(KEY_ESCAPE))
		{
			running = false;
			break;
		}

		render();
	}
}

void render()
{
	//acquire an image from the swap chain
	uint32_t imageIndex;

	//using uint64 max for timeout disables it
	VkResult res = vkAcquireNextImageKHR(appContext.device, appContext.swapChain.swapChain, UINT64_MAX, appContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkh::waitForFence(appContext.frameFences[imageIndex], appContext.device);
	vkResetFences(appContext.device, 1, &appContext.frameFences[imageIndex]);

	//record drawing
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional
	vkResetCommandBuffer(demoData.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	res = vkBeginCommandBuffer(demoData.commandBuffers[imageIndex], &beginInfo);


	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = demoData.mainRenderPass;
	renderPassInfo.framebuffer = demoData.frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = appContext.swapChain.extent;

	std::vector<VkClearValue> clearColors;

	//color
	clearColors.push_back({ 0.0f, 0.0f, 0.0f, 1.0f });

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = &clearColors[0];
	vkCmdBeginRenderPass(demoData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(demoData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, demoData.pipelineLayout[0], 0, 1, &demoData.descriptorSet, 0, 0);

	for (uint32_t i = 0; i < 4; ++i)
	{
		uint32_t material = i % 2;

		vkCmdBindPipeline(demoData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, demoData.graphicsPipeline[material]);

		int arrayIdx = i;

		vkCmdPushConstants(
			demoData.commandBuffers[imageIndex],
			demoData.pipelineLayout[material],
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(int),
			(void*)&arrayIdx);


		VkBuffer vertexBuffers[] = { demoData.quadMeshes[i].vBuffer };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(demoData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
		vkCmdBindIndexBuffer(demoData.commandBuffers[imageIndex], demoData.quadMeshes[i].iBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(demoData.commandBuffers[imageIndex], static_cast<uint32_t>(demoData.quadMeshes[i].iCount), 1, 0, 0, 0);

	}

	vkCmdEndRenderPass(demoData.commandBuffers[imageIndex]);
	res = vkEndCommandBuffer(demoData.commandBuffers[imageIndex]);
	assert(res == VK_SUCCESS);


	//submit

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//wait on writing colours to the buffer until the semaphore says the buffer is available
	VkSemaphore waitSemaphores[] = { appContext.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;

	VkSemaphore signalSemaphores[] = { appContext.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = &demoData.commandBuffers[imageIndex];
	submitInfo.commandBufferCount = 1;

	res = vkQueueSubmit(appContext.deviceQueues.graphicsQueue, 1, &submitInfo, appContext.frameFences[imageIndex]);
	assert(res == VK_SUCCESS);

	//present

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { appContext.swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional
	res = vkQueuePresentKHR(appContext.deviceQueues.transferQueue, &presentInfo);
}

void shutdown()
{

}