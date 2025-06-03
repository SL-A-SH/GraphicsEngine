#include "systemmanager.h"
#include "../../Core/System/Logger.h"
#include <sstream>

SystemManager::SystemManager()
{
	m_Input = 0;
	m_Application = 0;
}

SystemManager::SystemManager(const SystemManager& other)
{
}

SystemManager::~SystemManager()
{
}

bool SystemManager::Initialize()
{
	int screenWidth, screenHeight;
	bool result;

	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenWidth = 0;
	screenHeight = 0;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create and initialize the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new InputManager;

	result = m_Input->Initialize(m_hinstance, m_hwnd, screenWidth, screenHeight);
	if (!result)
	{
		LOG_ERROR("Failed to initialize InputManager");
		return false;
	}

	// Create and initialize the application class object.  This object will handle rendering all the graphics for this application.
	m_Application = new Application;

	result = m_Application->Initialize(screenWidth, screenHeight, m_hwnd);
	if (!result)
	{
		LOG_ERROR("Failed to initialize Application");
		return false;
	}

	LOG("SystemManager initialized successfully");
	return true;
}

void SystemManager::Shutdown()
{
	// Release the application class object.
	if (m_Application)
	{
		m_Application->Shutdown();
		delete m_Application;
		m_Application = 0;
	}

	// Release the input object.
	if (m_Input)
	{
		delete m_Input;
		m_Input = 0;
	}

	// Shutdown the window.
	ShutdownWindows();

	return;
}

void SystemManager::Run()
{
	MSG msg;
	bool done, result;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	done = false;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}
	}

	return;
}

bool SystemManager::Frame()
{
	bool result;

	// Do the input frame processing.
	result = m_Input->Frame();
	if (!result)
	{
		return false;
	}

	// Do the frame processing for the application class object.
	result = m_Application->Frame(m_Input);
	if (!result)
	{
		return false;
	}

	return true;
}

LRESULT CALLBACK SystemManager::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void SystemManager::InitializeWindows(int& screenWidth, int& screenHeight)
{
	// Get the instance of this application.
	m_hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	m_applicationName = L"DirectX11 Engine";

	// Get the window dimensions
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	screenWidth = rect.right - rect.left;
	screenHeight = rect.bottom - rect.top;

	std::stringstream ss;
	ss << "Window initialized with size: " << screenWidth << "x" << screenHeight;
	LOG(ss.str());
}

void SystemManager::ShutdownWindows()
{
	// Release the pointer to this class.
	ApplicationHandle = NULL;
}

