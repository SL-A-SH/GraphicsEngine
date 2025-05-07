#ifndef _APPLICATIONCLASS_H_
#define _APPLICATIONCLASS_H_

#include "../Graphics/D3DClass.h"
#include "../Graphics/CameraClass.h"
#include "../Graphics/ModelClass.h"
#include "../Graphics/Shaders/LightShaderClass.h"
#include "../Graphics/LightClass.h"
#include "../Graphics/BitmapClass.h"
#include "../Graphics/Shaders/TextureShaderClass.h"

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
	BitmapClass* m_Bitmap;
	TextureShaderClass* m_TextureShader;
};

#endif