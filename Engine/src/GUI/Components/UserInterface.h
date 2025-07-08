#ifndef _USERINTERFACE_H_
#define _USERINTERFACE_H_

#include <d3d11.h>
#include <directxmath.h>
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Shaders/Management/ShaderManager.h"
#include "../../Graphics/Rendering/Font.h"
#include "../../Graphics/Resource/Text.h"

using namespace DirectX;

class UserInterface
{
public:
    UserInterface();
    UserInterface(const UserInterface& other);
    ~UserInterface();

    bool Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth);
    void Shutdown();
    bool Frame(ID3D11DeviceContext* deviceContext, int fps, int renderCount);
    bool Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix);
    void SetShowFps(bool show) { m_ShowFps = show; }

private:
    bool UpdateFpsString(ID3D11DeviceContext* deviceContext, int fps);
    bool UpdateRenderCountString(ID3D11DeviceContext* deviceContext, int renderCount);

private:
    Font* m_Font;
    Text* m_FpsString;
    Text* m_RenderCountString;
    int m_screenWidth, m_screenHeight, m_QTOffset;
    int m_previousFps;
    int m_previousRenderCount;
    bool m_ShowFps;
};

#endif 