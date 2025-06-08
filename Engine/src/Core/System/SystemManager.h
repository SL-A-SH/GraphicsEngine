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
	bool Frame();
	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
	void SetWindowHandle(HWND);

private:
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	InputManager* m_Input;
	Application* m_Application;

public:
	Application* GetApplication() { return m_Application; }
	InputManager* GetInputManager() { return m_Input; }

};

static SystemManager* ApplicationHandle = 0;

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#endif