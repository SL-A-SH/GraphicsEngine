#pragma once

#include "../../D3D11/D3D11Device.h"
#include "../../Resource/Environment/SkyDome.h"
#include "../../Shaders/ShaderManager.h"
#include "../../Rendering/Camera.h"

class Zone
{
public:
	Zone();
	Zone(const Zone&);
	~Zone();

	bool Initialize(ID3D11Device*);
	void Shutdown();
	bool Render(D3D11Device*, ShaderManager*, Camera*);

private:
	SkyDome* m_SkyDome;
};