#include "zone.h"


Zone::Zone()
{
	m_SpaceSkybox = 0;
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

	// Create the space skybox object.
	m_SpaceSkybox = new SpaceSkybox;
	if (!m_SpaceSkybox)
	{
		return false;
	}

	// Initialize the space skybox object.
	result = m_SpaceSkybox->Initialize(device, deviceContext);
	if (!result)
	{
		return false;
	}

	return true;
}


void Zone::Shutdown()
{
	// Release the space skybox object.
	if (m_SpaceSkybox)
	{
		m_SpaceSkybox->Shutdown();
		delete m_SpaceSkybox;
		m_SpaceSkybox = 0;
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

	// Update the space skybox time for animation
	float deltaTime = 0.016f; // Approximate 60 FPS
	m_SpaceSkybox->UpdateTime(deltaTime);

	// Main star parameters
	float mainStarSize = 0.1f;
	XMFLOAT3 mainStarDir = XMFLOAT3(0.0f, 0.0f, 1.0f);
	XMFLOAT3 mainStarColor = XMFLOAT3(1.0f, 0.9f, 0.8f);
	float mainStarIntensity = 10.0f;

	// Render the space skybox using the space skybox shader.
	m_SpaceSkybox->Render(Direct3D->GetDeviceContext());
	result = ShaderManager->RenderSpaceSkyboxShader(Direct3D->GetDeviceContext(), m_SpaceSkybox->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_SpaceSkybox->GetTime(), mainStarSize, mainStarDir, mainStarColor, mainStarIntensity);
	if (!result)
	{
		return false;
	}

	return true;
}
