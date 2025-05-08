#ifndef _APPLICATIONCLASS_H_
#define _APPLICATIONCLASS_H_

#include "../../Graphics/D3D11/D3DClass.h"
#include "../../Graphics/Rendering/Camera.h"
#include "../../Graphics/Rendering/Light.h"
#include "../../Graphics/Resource/Model.h"
#include "../../Graphics/Shaders/LightShader.h"
#include "../../Graphics/Rendering/Sprite.h"
#include "../../Core/System/Timer.h"
#include "../../Graphics/Shaders/TextureShader.h"

/////////////
// GLOBALS //
/////////////
const bool FULL_SCREEN = true;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

class ApplicationClass
{
public:
	ApplicationClass();
	ApplicationClass(const ApplicationClass&);
	~ApplicationClass();

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame();

private:
	bool Render();

private:
	D3DClass* m_Direct3D;
	CameraClass* m_Camera;
	ModelClass* m_Model;
	LightShaderClass* m_LightShader;
	LightClass* m_Light;
	Sprite* m_Sprite;
	Timer* m_Timer;
	TextureShaderClass* m_TextureShader;
};

#endif