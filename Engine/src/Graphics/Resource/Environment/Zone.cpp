#include "zone.h"


Zone::Zone()
{
	m_SkyDome = 0;
}


Zone::Zone(const Zone& other)
{
}


Zone::~Zone()
{
}


bool Zone::Initialize(ID3D11Device* device)
{
	bool result;

	// Create the sky dome object.
	m_SkyDome = new SkyDome;
	if (!m_SkyDome)
	{
		return false;
	}

	// Initialize the sky dome object.
	result = m_SkyDome->Initialize(device);
	if (!result)
	{
		return false;
	}

	return true;
}


void Zone::Shutdown()
{
	// Release the sky dome object.
	if (m_SkyDome)
	{
		m_SkyDome->Shutdown();
		delete m_SkyDome;
		m_SkyDome = 0;
	}

	return;
}


bool Zone::Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, Camera* camera)
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
	bool result;
	XMFLOAT3 cameraPosition;

	// Get the position of the camera.
	cameraPosition = camera->GetPosition();

	// Translate the sky dome to be centered around the camera position.
	worldMatrix = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// Get the view and projection matrices from the camera and d3d objects.
	camera->GetViewMatrix(viewMatrix);
	Direct3D->GetProjectionMatrix(projectionMatrix);

	// Render the sky dome using the sky dome shader.
	m_SkyDome->Render(Direct3D->GetDeviceContext());
	result = ShaderManager->RenderSkyDomeShader(Direct3D->GetDeviceContext(), m_SkyDome->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_SkyDome->GetApexColor(), m_SkyDome->GetCenterColor());
	if (!result)
	{
		return false;
	}

	return true;
}
