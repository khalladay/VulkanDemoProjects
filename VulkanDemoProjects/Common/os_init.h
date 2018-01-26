#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>

#include <atlstr.h>

namespace OS
{
	struct AppInfo
	{
		struct HWND__* wndHdl;
		struct HINSTANCE__* instance;
		void(*resizeCallback)(int, int) = nullptr;
		double initialMS = 0.0;
		int curW;
		int curH;
	};

	AppInfo GAppInfo;

	void setResizeCallback(void(*cb)(int, int))
	{
		GAppInfo.resizeCallback = cb;
	}

	LRESULT CALLBACK defaultWndFunc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = 0;
		switch (message)
		{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}break;
		case WM_SIZE:
		{
			RECT rect;
			GetWindowRect(window, &rect);
			GAppInfo.curW = rect.right - rect.left;
			GAppInfo.curH = rect.bottom - rect.top;

			if (GAppInfo.resizeCallback)
			{
				GAppInfo.resizeCallback(GAppInfo.curW, GAppInfo.curH);
			}
		}
		default:
		{
			result = DefWindowProc(window, message, wParam, lParam);
		}break;
		}

		return result;
	}

	double getMilliseconds()
	{
		static LARGE_INTEGER s_frequency;
		static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
		if (s_use_qpc) {
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			return (1000LL * now.QuadPart) / (double)s_frequency.QuadPart - GAppInfo.initialMS;
		}
		else {
			return GetTickCount64() - GAppInfo.initialMS;
		}
	}

	void handleEvents()
	{
		MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	HWND makeWindow(HINSTANCE Instance, const char* title, unsigned int width, unsigned int height)
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		FILE* pCout;
		freopen_s(&pCout, "conout$", "w", stdout);
		freopen_s(&pCout, "conin$", "w", stdin);
		freopen_s(&pCout, "conout$", "w", stderr);

		fclose(pCout);

		WNDCLASS windowClass = {};
		windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = defaultWndFunc;
		windowClass.hInstance = Instance;
		windowClass.lpszClassName = title;

		RegisterClass(&windowClass);
		HWND wndHdl = CreateWindowEx(0,
			windowClass.lpszClassName,
			title,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			0,
			0,
			Instance,
			0);

		GAppInfo.instance = Instance;
		GAppInfo.wndHdl = wndHdl;
		GAppInfo.curH = height;
		GAppInfo.curW = width;
		GAppInfo.initialMS = getMilliseconds();

		return wndHdl;
	}

}
