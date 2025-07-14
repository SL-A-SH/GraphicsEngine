#pragma once

#include "../../D3D11/D3D11Device.h"
#include "../../Resource/Environment/SpaceSkybox.h"
#include "../../Shaders/Management/ShaderManager.h"
#include "../../Rendering/Camera.h"

class Zone
{
public:
	Zone();
	Zone(const Zone&);
	~Zone();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*);
	void Shutdown();
	bool Render(D3D11Device*, ShaderManager*, Camera*);

private:
	SpaceSkybox* m_SpaceSkybox;
};