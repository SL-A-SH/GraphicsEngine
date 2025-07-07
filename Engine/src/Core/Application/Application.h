#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#define DIRECTINPUT_VERSION 0x0800

#include <d3d11.h>
#include <directxmath.h>
#include <dinput.h>
#include <functional>

#include "../Input/InputManager.h"
#include "../../Core/System/Timer.h"
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Rendering/Camera.h"
#include "../../Graphics/Rendering/Light.h"
#include "../../Graphics/Resource/Model.h"
#include "../../Graphics/Rendering/Sprite.h"
#include "../../Graphics/Rendering/Font.h"
#include "../../Graphics/Resource/Text.h"
#include "../../Graphics/Shaders/ShaderManager.h"
#include "../../Graphics/Scene/ModelList.h"
#include "../../Graphics/Scene/SelectionManager.h"
#include "../../Graphics/Math/Frustum.h"
#include "../../Graphics/Math/Position.h"
#include "../../Graphics/Resource/Environment/Zone.h"
#include "../../Graphics/UI/UserInterface.h"
#include "../../Graphics/Rendering/DisplayPlane.h"
#include "../../Core/System/PerformanceProfiler.h"

using namespace DirectX;

// Globals
const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class MainWindow;

class Application
{
public:
	Application();
	Application(const Application&);
	~Application();

	bool Initialize(int, int, HWND, MainWindow*);
	void Shutdown();
	bool Frame(InputManager*);
	bool Resize(int, int);

	// Getters for UI components
	UserInterface* GetUserInterface() { return m_UserInterface; }
	SelectionManager* GetSelectionManager() { return m_SelectionManager; }
	
	// Set UI switching callbacks
	void SetUISwitchingCallbacks(std::function<void()> switchToModelList, std::function<void()> switchToTransformUI);

private:
	bool Render();
	bool UpdateFps();
	bool UpdateRenderCountString(int);

private:
	D3D11Device* m_Direct3D;
	MainWindow* m_mainWindow;
	Camera* m_Camera;
	Model* m_Model;
	Model* m_Floor;
	Light* m_Light;
	ShaderManager* m_ShaderManager;
	Zone* m_Zone;
	Sprite* m_Cursor;
	Timer* m_Timer;
	Font* m_Font;
	Text* m_FpsString;
	int m_previousFps;
	Text* m_RenderCountString;
	ModelList* m_ModelList;
	Position* m_Position;
	Frustum* m_Frustum;
	UserInterface* m_UserInterface;

	int m_screenWidth, m_screenHeight;
	int m_Fps;
	int m_RenderCount;
	XMMATRIX m_baseViewMatrix;
	XMMATRIX m_projectionMatrix;
	XMMATRIX m_orthoMatrix;

	// New components for selection and transformation
	SelectionManager* m_SelectionManager;
	
	// UI switching callbacks
	std::function<void()> m_switchToModelListCallback;
	std::function<void()> m_switchToTransformUICallback;
	
	// Gizmo models
	Model* m_PositionGizmo;
	Model* m_RotationGizmo;
	Model* m_ScaleGizmo;
	
	DisplayPlane* m_DisplayPlane;
};

#endif