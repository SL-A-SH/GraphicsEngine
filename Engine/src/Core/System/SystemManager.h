#ifndef _SYSTEMMANAGER_H_
#define _SYSTEMMANAGER_H_

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "../Input/InputManager.h"
#include "../Application/Application.h"

class SystemManager
{
public:
	SystemManager();
	SystemManager(const SystemManager&);
	~SystemManager();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	InputManager* m_Input;
	Application* m_Application;
};

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


/////////////
// GLOBALS //
/////////////
static SystemManager* ApplicationHandle = 0;


#endif