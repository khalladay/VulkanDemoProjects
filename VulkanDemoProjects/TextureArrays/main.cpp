#include "os_support.h"
#include "os_input.h"
#include "timing.h"
#include <stdio.h>

void mainLoop();
void shutdown();
void logFPSAverage(double avg);

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE pInstance, LPSTR cmdLine, int showCode)
{
	HWND wndHdl = os_makeWindow(Instance, "Texture Arrays", 800, 600);
	os_initializeInput();

	mainLoop();
	shutdown();

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

	fpsData.logCallback = logFPSAverage;

	startTimingFrame(fpsData);

	while (running)
	{
		double dt = endTimingFrame(fpsData);
		startTimingFrame(fpsData);

		os_handleEvents();
		os_pollInput();

		if (getKey(KEY_ESCAPE))
		{
			running = false;
			break;
		}

	}
}

void shutdown()
{
	os_shutdownInput();
}