#include "zone.h"


Zone::Zone()
{
	m_Skybox = 0;
}


Zone::Zone(const Zone& other)
{
}


Zone::~Zone()
{
}


bool Zone::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	bool result;

	// Create the skybox object.
	m_Skybox = new Skybox;
	if (!m_Skybox)
	{
		return false;
	}

	// Initialize the skybox object.
	result = m_Skybox->Initialize(device, deviceContext);
	if (!result)
	{
		return false;
	}

	return true;
}


void Zone::Shutdown()
{
	// Release the skybox object.
	if (m_Skybox)
	{
		m_Skybox->Shutdown();
		delete m_Skybox;
		m_Skybox = 0;
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

	// Translate the skybox to be centered around the camera position.
	worldMatrix = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// Get the view and projection matrices from the camera and d3d objects.
	camera->GetViewMatrix(viewMatrix);
	Direct3D->GetProjectionMatrix(projectionMatrix);

	// Render the skybox using the skybox shader.
	m_Skybox->Render(Direct3D->GetDeviceContext());
	result = ShaderManager->RenderSkyboxShader(Direct3D->GetDeviceContext(), m_Skybox->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_Skybox->GetTextureArray());
	if (!result)
	{
		return false;
	}

	return true;
}
