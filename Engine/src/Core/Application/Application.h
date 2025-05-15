#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "../../Graphics/D3D11/D3D11Device.h"
#include "../Input/InputManager.h"
#include "../../Graphics/Rendering/Camera.h"
#include "../../Graphics/Rendering/Light.h"
#include "../../Graphics/Resource/Model.h"
#include "../../Graphics/Shaders/LightShader.h"
#include "../../Graphics/Rendering/Sprite.h"
#include "../../Core/System/Timer.h"
#include "../../Graphics/Shaders/TextureShader.h"
#include "../../Graphics/Rendering/Font.h"
#include "../../Graphics//Resource/Text.h"
#include "../../Graphics/Shaders/FontShader.h"

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
	bool UpdateMouseStrings(int, int, bool);
	bool UpdateFps();

private:
	D3D11Device* m_Direct3D;
	Camera* m_Camera;
	Model* m_Model;
	LightShader* m_LightShader;
	Light* m_Light;
	Sprite* m_Cursor;
	Timer* m_Timer;
	TextureShader* m_TextureShader;
	FontShader* m_FontShader;
	Font* m_Font;
	Text* m_TextString1, * m_TextString2;
	Text* m_MouseStrings;
	Text* m_FpsString;
	int m_previousFps;
};

#endif