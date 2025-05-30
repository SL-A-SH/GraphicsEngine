#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "../Input/InputManager.h"
#include "../../Core/System/Timer.h"
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Rendering/Camera.h"
#include "../../Graphics/Rendering/Light.h"
#include "../../Graphics/Resource/Model.h"
#include "../../Graphics/Rendering/Sprite.h"
#include "../../Graphics/Rendering/Font.h"
#include "../../Graphics//Resource/Text.h"
#include "../../Graphics/Shaders/ShaderManager.h"
#include "../../Graphics/Scene/ModelList.h"
#include "../../Graphics/Math/Frustum.h"
#include "../../Graphics/Math/Position.h"
#include "../../Graphics/Resource/Environment/Zone.h"

const bool FULL_SCREEN = true;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

class Application
{
public:
	Application();
	Application(const Application&);
	~Application();

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame(InputManager*);

private:
	bool Render();
	bool UpdateFps();
	bool UpdateRenderCountString(int);

private:
	D3D11Device* m_Direct3D;
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
	XMMATRIX m_baseViewMatrix;

	int m_screenWidth, m_screenHeight;
};

#endif