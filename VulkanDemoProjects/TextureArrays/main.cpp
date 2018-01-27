#include "Common.h"

vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMesh;
};

DemoData demoData;

void mainLoop();
void shutdown();

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

	vkh::Mesh::quad(demoData.quadMesh, appContext);

	mainLoop();

	return 0;
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
	}

	shutdown();
}

void shutdown()
{
	OS::shutdownInput();
}

