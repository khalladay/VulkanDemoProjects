#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "os_init.h"
#include "debug.h"

#define checkhf(expr, format, ...) if (FAILED(expr))														\
{																											\
    fprintf(stdout, "CHECK FAILED: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);	\
    CString dbgstr;																							\
    dbgstr.Format(("%s"), format);																			\
    MessageBox(NULL,dbgstr, "FATAL ERROR", MB_OK);															\
}

//these map to directInput keycodes
enum KeyCode
{
	KEY_ESCAPE = 0x01,
	KEY_1 = 0x02,
	KEY_2 = 0x03,
	KEY_3 = 0x04,
	KEY_4 = 0x05,
	KEY_5 = 0x06,
	KEY_6 = 0x07,
	KEY_7 = 0x08,
	KEY_8 = 0x09,
	KEY_9 = 0x0A,
	KEY_0 = 0x0B,
	KEY_MINUS = 0x0C,
	KEY_EQUALS = 0x0D,
	KEY_BACKSPACE = 0x0E,
	KEY_TAB = 0x0F,
	KEY_Q = 0x10,
	KEY_W = 0x11,
	KEY_E = 0x12,
	KEY_R = 0x13,
	KEY_T = 0x14,
	KEY_Y = 0x15,
	KEY_U = 0x16,
	KEY_I = 0x17,
	KEY_O = 0x18,
	KEY_P = 0x19,
	KEY_LBRACKET = 0x1A,
	KEY_RBRACKET = 0x1B,
	KEY_RETURN = 0x1C,
	KEY_LCTRL = 0x1D,
	KEY_A = 0x1E,
	KEY_S = 0x1F,
	KEY_D = 0x20,
	KEY_F = 0x21,
	KEY_G = 0x22,
	KEY_H = 0x23,
	KEY_J = 0x24,
	KEY_K = 0x25,
	KEY_L = 0x26,
	KEY_SEMICOLON = 0x27,
	KEY_GRAVE = 0x29,
	KEY_APOSTROPHE = 0x28,
	KEY_LSHIFT = 0x2A,
	KEY_BACKKSLASH = 0x2B,
	KEY_Z = 0x2C,
	KEY_X = 0x2D,
	KEY_C = 0x2E,
	KEY_V = 0x2F,
	KEY_B = 0x30,
	KEY_N = 0x31,
	KEY_M = 0x32,
	KEY_COMMA = 0x33,
	KEY_PERIOD = 0x34,
	KEY_SLASH = 0x35,
	KEY_RSHIFT = 0x36,
	KEY_SPACE = 0x39,
	KEY_CAPS = 0x3A,
	KEY_F1 = 0x3B,
	KEY_F2 = 0x3C,
	KEY_F3 = 0x3D,
	KEY_F4 = 0x3E,
	KEY_F5 = 0x3F,
	KEY_F6 = 0x40,
	KEY_F7 = 0x41,
	KEY_F8 = 0x42,
	KEY_F9 = 0x43,
	KEY_F10 = 0x44,
	KEY_F11 = 0x57,
	KEY_F12 = 0x58,
	KEY_UP = 0xC8,
	KEY_LEFT = 0xCB,
	KEY_RIGHT = 0xCD,
	KEY_DOWN = 0xD0
};

namespace OS
{
	struct InputState
	{
		int screenWidth;
		int screenHeight;
		int mouseX;
		int mouseY;
		int mouseDX;
		int mouseDY;

		unsigned char keyStates[256];
		DIMOUSESTATE mouseState;

		IDirectInput8* diDevice;
		IDirectInputDevice8* diKeyboard;
		IDirectInputDevice8* diMouse;

	};

	InputState GInputState;


	void initializeInput()
	{
		HRESULT result;

		GInputState.screenHeight = GAppInfo.curH;
		GInputState.screenWidth = GAppInfo.curH;
		GInputState.mouseX = 0;
		GInputState.mouseY = 0;

		// Initialize the main direct input interface.
		result = DirectInput8Create(GAppInfo.instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&GInputState.diDevice, NULL);
		checkhf(result, "Failed to create DirectInput device");

		// Initialize the direct input interface for the keyboard.
		result = GInputState.diDevice->CreateDevice(GUID_SysKeyboard, &GInputState.diKeyboard, NULL);
		checkhf(result, "Failed to create DirectInput keyboard");

		// Set the data format.  In this case since it is a keyboard we can use the predefined data format.
		result = GInputState.diKeyboard->SetDataFormat(&c_dfDIKeyboard);
		checkhf(result, "Failed to set data format for DirectInput kayboard");

		// Set the cooperative level of the keyboard to not share with other programs.
		result = GInputState.diKeyboard->SetCooperativeLevel(GAppInfo.wndHdl, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
		checkhf(result, "Failed to set keyboard to foreground exclusive");

		// Now acquire the keyboard.
		result = GInputState.diKeyboard->Acquire();
		checkhf(result, "Failed to acquire DI Keyboard");

		// Initialize the direct input interface for the mouse.
		result = GInputState.diDevice->CreateDevice(GUID_SysMouse, &GInputState.diMouse, NULL);
		checkhf(result, "Failed ot create DirectInput keyboard");

		// Set the data format for the mouse using the pre-defined mouse data format.
		result = GInputState.diMouse->SetDataFormat(&c_dfDIMouse);
		checkhf(result, "Failed to set data format for DirectInput mouse");

		//result = GInputState.diMouse->SetCooperativeLevel(wndHdl, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
		//checkhf(result, "Failed to get exclusive access to DirectInput mouse");

		// Acquire the mouse.
		result = GInputState.diMouse->Acquire();
		checkhf(result, "Failed to acquire DirectInput mouse");
	}

	void pollInput()
	{
		HRESULT result;

		// Read the keyboard device.
		result = GInputState.diKeyboard->GetDeviceState(sizeof(GInputState.keyStates), (LPVOID)&GInputState.keyStates);
		if (FAILED(result))
		{
			// If the keyboard lost focus or was not acquired then try to get control back.
			if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
			{
				GInputState.diKeyboard->Acquire();
			}
			else
			{
				checkf(0, "Error polling keyboard");
			}
		}

		// Read the mouse device.
		result = GInputState.diMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&GInputState.mouseState);
		if (FAILED(result))
		{
			// If the mouse lost focus or was not acquired then try to get control back.
			if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
			{
				GInputState.diMouse->Acquire();
			}
			else
			{
				checkf(0, "Error polling mouse");
			}
		}

		// Update the location of the mouse cursor based on the change of the mouse location during the frame.
		GInputState.mouseX += GInputState.mouseState.lX;
		GInputState.mouseY += GInputState.mouseState.lY;
		GInputState.mouseDX = GInputState.mouseState.lX;
		GInputState.mouseDY = GInputState.mouseState.lY;

		// Ensure the mouse location doesn't exceed the screen width or height.
		if (GInputState.mouseX < 0) { GInputState.mouseX = 0; }
		if (GInputState.mouseY < 0) { GInputState.mouseY = 0; }

		if (GInputState.mouseX > GInputState.screenWidth) { GInputState.mouseX = GInputState.screenWidth; }
		if (GInputState.mouseY > GInputState.screenHeight) { GInputState.mouseY = GInputState.screenHeight; }
	}

	void shutdownInput()
	{
		if (GInputState.diMouse)
		{
			GInputState.diMouse->Unacquire();
			GInputState.diMouse->Release();
			GInputState.diMouse = 0;
		}

		// Release the keyboard.
		if (GInputState.diKeyboard)
		{
			GInputState.diKeyboard->Unacquire();
			GInputState.diKeyboard->Release();
			GInputState.diKeyboard = 0;
		}

		// Release the main interface to direct input.
		if (GInputState.diDevice)
		{
			GInputState.diDevice->Release();
			GInputState.diDevice = 0;
		}
	}

	bool getKey(KeyCode key)
	{
		return GInputState.keyStates[key] > 0;
	}

	int getMouseDX()
	{
		return GInputState.mouseDX;
	}

	int getMouseDY()
	{
		return GInputState.mouseDY;
	}

	int getMouseX()
	{
		return GInputState.mouseX;
	}

	int getMouseY()
	{
		return GInputState.mouseY;
	}

	int getMouseLeftButton()
	{
		return GInputState.mouseState.rgbButtons[0] > 0;
	}

	int getMouseRightButton()
	{
		return GInputState.mouseState.rgbButtons[2] > 0;
	}

	//debating if these initialization functions should be in 
	//os_support or not, but I don't want to expose the InputState struct
	//in a header. 
	void os_initializeInput();
	void os_pollInput();
	void os_shutdownInput();

	bool getKey(KeyCode key);
	int getMouseDX();
	int getMouseDY();
	int getMouseX();
	int getMouseY();
	int getMouseLeftButton();
	int getMouseRightButton();
}