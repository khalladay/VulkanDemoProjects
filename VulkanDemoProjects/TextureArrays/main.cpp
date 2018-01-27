#include "Common.h"

vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMesh;
	vkh::TextureAsset textures[8];

	std::vector<VkFramebuffer>		frameBuffers;
	vkh::VkhRenderBuffer			depthBuffer;
	std::vector<VkCommandBuffer>	commandBuffers;

	VkRenderPass					mainRenderPass;

	//We only have 1 material, so this can be stored here.
	VkPipelineLayout				pipelineLayout;
	VkDescriptorSet					descriptorSet;

};

DemoData demoData;

void mainLoop();
void shutdown();
void createMainRenderPass();
void render();
void setupShader();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE pInstance, LPSTR cmdLine, int showCode)
{
	HWND wndHdl = OS::makeWindow(Instance, "Texture Array Demo", 800, 600);
	OS::initializeInput();

	vkh::VkhContextCreateInfo ctxtInfo = {};
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
	ctxtInfo.typeCounts.push_back(8);
	ctxtInfo.typeCounts.push_back(1);

	initContext(ctxtInfo, "Texture Array Demo", Instance, wndHdl, appContext);
	createMainRenderPass();
	vkh::createFrameBuffers(demoData.frameBuffers, appContext.swapChain,nullptr, demoData.mainRenderPass, appContext.device);

	uint32_t swapChainImageCount = static_cast<uint32_t>(appContext.swapChain.imageViews.size());
	demoData.commandBuffers.resize(swapChainImageCount);
	for (uint32_t i = 0; i < swapChainImageCount; ++i)
	{
		vkh::createCommandBuffer(demoData.commandBuffers[i], appContext.gfxCommandPool, appContext.device);
	}

	vkh::Mesh::quad(demoData.quadMesh, appContext);

	for (uint32_t i = 0; i < 8; ++i)
	{
		char filename[32];
		sprintf_s(filename, 32, "textures\\%i.png", i);
		vkh::Texture::make(demoData.textures[i], filename, appContext);
	}

	mainLoop();

	return 0;
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

void setupShader()
{

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

	shutdown();
}

void shutdown()
{
	OS::shutdownInput();
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
	clearColors.push_back({ 1.0f, 0.0f, 0.0f, 1.0f });

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = &clearColors[0];
	vkCmdBeginRenderPass(demoData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


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
