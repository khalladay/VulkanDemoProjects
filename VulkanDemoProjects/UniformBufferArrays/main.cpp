#include "Common.h"

vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMeshes[2];

	std::vector<VkFramebuffer>		frameBuffers;
	std::vector<VkCommandBuffer>	commandBuffers;

	VkRenderPass					mainRenderPass;

	VkDescriptorSet					descriptorSet;
	VkDescriptorSetLayout			descSetLayout;

	VkPipelineLayout				pipelineLayout[2];
	VkPipeline						graphicsPipeline[2];
};

DemoData demoData;

void setupDemo();
void createMainRenderPass();
void mainLoop();
void shutdown();
void logFPSAverage(double avg);
void render();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE pInstance, LPSTR cmdLine, int showCode)
{
	HWND wndHdl = OS::makeWindow(Instance, "Uniform Buffer Array Demo", 800, 600);
	OS::initializeInput();

	vkh::VkhContextCreateInfo ctxtInfo = {};
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
	ctxtInfo.typeCounts.push_back(8);
	ctxtInfo.typeCounts.push_back(1);

	initContext(ctxtInfo, "Uniform Buffer Array Demo", Instance, wndHdl, appContext);

	mainLoop();
	shutdown();

	return 0;
}

void setupDemo()
{
	vkh::Mesh::quad(demoData.quadMeshes[0], appContext, 1.0f, 2.0f, -1.0f, 0.0f);
	vkh::Mesh::quad(demoData.quadMeshes[1], appContext, 1.0f, 2.0f, 1.0f, 0.0f);

	vkh::createFrameBuffers(demoData.frameBuffers, appContext.swapChain, nullptr, demoData.mainRenderPass, appContext.device);

	uint32_t swapChainImageCount = static_cast<uint32_t>(appContext.swapChain.imageViews.size());
	demoData.commandBuffers.resize(swapChainImageCount);
	for (uint32_t i = 0; i < swapChainImageCount; ++i)
	{
		vkh::createCommandBuffer(demoData.commandBuffers[i], appContext.gfxCommandPool, appContext.device);
	}

	createMainRenderPass();

	vkh::VkhMaterialCreateInfo createInfo = {};
	createInfo.renderPass = demoData.mainRenderPass;
	createInfo.outPipeline = &demoData.graphicsPipeline[0];
	createInfo.outPipelineLayout = &demoData.pipelineLayout[0];
	createInfo.descSetLayouts.push_back(demoData.descSetLayout);

	vkh::createBasicMaterial("shader\\common_vert.vert", "shaders\\frag1.frag", appContext, createInfo);

	vkh::VkhMaterialCreateInfo createInfo2 = {};
	createInfo2.renderPass = demoData.mainRenderPass;
	createInfo2.outPipeline = &demoData.graphicsPipeline[1];
	createInfo2.outPipelineLayout = &demoData.pipelineLayout[1];
	createInfo2.descSetLayouts.push_back(demoData.descSetLayout);

	vkh::createBasicMaterial("shader\\common_vert.vert", "shaders\\frag2.frag", appContext, createInfo2);

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

}

void shutdown()
{

}