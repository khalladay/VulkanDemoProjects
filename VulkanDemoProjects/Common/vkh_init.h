#pragma once
#include "vkh_types.h"
#include "vkh.h"

namespace vkh
{
	void initContext(VkhContext& ctxt, const char* appName, HINSTANCE Instance, HWND wndHdl)
	{
		createInstance(ctxt, appName);
		createDebugCallback(ctxt);

		createWin32Surface(ctxt, Instance, wndHdl);
		createPhysicalDevice(ctxt);
		createLogicalDevice(ctxt);

	}
}