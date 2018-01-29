#include "Common.h"

vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMeshes[2];

	std::vector<VkFramebuffer>		frameBuffers;
	std::vector<VkCommandBuffer>	commandBuffers;

	VkRenderPass					mainRenderPass;

	VkPipelineLayout				pipelineLayout;
	VkDescriptorSet					descriptorSet;
	VkPipeline						graphicsPipeline;
	VkDescriptorSetLayout			descSetLayout;
};

DemoData demoData;

void setupDemo();
void mainLoop();
void shutdown();
void logFPSAverage(double avg);
void render();

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

	mainLoop();
	shutdown();

	return 0;
}

void setupDemo()
{
	vkh::Mesh::quad(demoData.quadMeshes[0], appContext, 1.0f, 2.0f, -1.0f, 0.0f);
	vkh::Mesh::quad(demoData.quadMeshes[1], appContext, 1.0f, 2.0f, 1.0f, 0.0f);
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