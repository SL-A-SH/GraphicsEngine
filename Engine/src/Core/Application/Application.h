#ifndef APPLICATION_H
#define APPLICATION_H

#include <d3d11.h>
#include <directxmath.h>
#include <functional>

// Forward declarations
class MainWindow;
class D3D11Device;
class Camera;
class Model;
class Light;
class ShaderManager;
class Zone;
class Sprite;
class Timer;
class Font;
class Text;
class ModelList;
class Position;
class Frustum;
class UserInterface;
class SelectionManager;
class DisplayPlane;
class GPUDrivenRenderer;
class PerformanceProfiler;
// class RenderingBenchmark; // DISABLED: Commented out for minimal GPU-driven rendering testing

using namespace DirectX;

// Application configuration constants
namespace AppConfig
{
    constexpr bool FULL_SCREEN = false;
    constexpr bool VSYNC_ENABLED = false;
    constexpr float SCREEN_DEPTH = 1000.0f;
    constexpr float SCREEN_NEAR = 0.1f;
}

class Application
{
public:
	Application();
	~Application();

	bool Initialize(int screenWidth, int screenHeight, HWND hwnd, MainWindow* mainWindow);
	void Shutdown();
	bool Frame(class InputManager* inputManager);
	bool Resize(int width, int height);

	// Getters for UI components
	UserInterface* GetUserInterface() { return m_UserInterface; }
	SelectionManager* GetSelectionManager() { return m_SelectionManager; }
	
	// Set UI switching callbacks
	void SetUISwitchingCallbacks(std::function<void()> switchToModelList, std::function<void()> switchToTransformUI);
	
	// GPU-driven rendering control
	void SetGPUDrivenRendering(bool enable) { m_enableGPUDrivenRendering = enable; }
	bool IsGPUDrivenRenderingEnabled() const { return m_enableGPUDrivenRendering; }
	
	// Get current rendering mode for profiling
	int GetCurrentRenderingMode() const 
	{ 
		return m_enableGPUDrivenRendering ? 1 : 0; // 0 = CPU_DRIVEN, 1 = GPU_DRIVEN
	}
	
	// Debug logging control
	void SetDebugLogging(bool enable) { m_debugLogging = enable; }
	bool IsDebugLoggingEnabled() const { return m_debugLogging; }

private:
	bool Render();
	bool UpdateFps();
	bool UpdateRenderCountString(int renderCount);

private:
	// Core systems
	D3D11Device* m_Direct3D;
	MainWindow* m_mainWindow;
	Camera* m_Camera;
	ShaderManager* m_ShaderManager;
	Timer* m_Timer;
	UserInterface* m_UserInterface;
	SelectionManager* m_SelectionManager;

	// Models and resources
	Model* m_Model;
	Model* m_PositionGizmo;
	Model* m_RotationGizmo;
	Model* m_ScaleGizmo;
	Light* m_Light;
	Zone* m_Zone;
	Sprite* m_Cursor;
	Font* m_Font;
	Text* m_FpsString;
	Text* m_RenderCountString;
	ModelList* m_ModelList;
	Position* m_Position;
	Frustum* m_Frustum;
	DisplayPlane* m_DisplayPlane;

	// Application state
	int m_screenWidth, m_screenHeight;
	int m_Fps;
	int m_RenderCount;
	int m_previousFps;
	XMMATRIX m_baseViewMatrix;
	XMMATRIX m_projectionMatrix;
	XMMATRIX m_orthoMatrix;

	// UI switching callbacks
	std::function<void()> m_switchToModelListCallback;
	std::function<void()> m_switchToTransformUICallback;
	
	// GPU-driven rendering
	GPUDrivenRenderer* m_GPUDrivenRenderer;
	bool m_enableGPUDrivenRendering;
	// RenderingBenchmark* m_BenchmarkSystem; // DISABLED: Commented out for minimal GPU-driven rendering testing
	
	// Debug logging
	bool m_debugLogging;
};

#endif