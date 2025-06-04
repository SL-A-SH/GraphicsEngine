#include "application.h"
#include "../../Core/System/Logger.h"


Application::Application()
{
	LOG("Application constructor called");
	m_Direct3D = 0;
	m_Camera = 0;
	m_Model = 0;
	m_Light = 0;
	m_ShaderManager = 0;
	m_Zone = 0;
	m_Timer = 0;
	m_ModelList = 0;
	m_Position = 0;
	m_Frustum = 0;
	m_Floor = 0;
	m_screenWidth = 0;
	m_screenHeight = 0;
	m_Fps = 0;
	m_RenderCount = 0;
	m_UserInterface = 0;
}


Application::Application(const Application& other)
{
}


Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	LOG("Application::Initialize called");
	char textureFilename1[128], textureFilename2[128], textureFilename3[128];
	char modelFilename[128];
	char floorTex[128];
	bool result;

	// Store screen dimensions
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	// Create and initialize the Direct3D object.
	LOG("Creating Direct3D object");
	m_Direct3D = new D3D11Device;

	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		LOG_ERROR("Could not initialize Direct3D");
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}
	LOG("Direct3D initialized successfully");

	// Create the camera object.
	LOG("Creating camera object");
	m_Camera = new Camera;

	// Set the initial position of the camera.
	m_Camera->SetPosition(0.0f, 0.0f, -300.0f);
	m_Camera->Render();
	m_Camera->GetViewMatrix(m_baseViewMatrix);
	LOG("Camera initialized successfully");

	// Create and initialize the zone object
	LOG("Creating zone object");
	m_Zone = new Zone;

	result = m_Zone->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext());
	if (!result)
	{
		LOG_ERROR("Could not initialize the zone object");
		MessageBox(hwnd, L"Could not initialize the zone object.", L"Error", MB_OK);
		return false;
	}
	LOG("Zone initialized successfully");

	// Set the file name of the model.
	strcpy_s(modelFilename, "../Engine/assets/models/Thriller.fbx");
	strcpy_s(textureFilename1, "../Engine/assets/textures/Stone02/stone02.tga");

	// Create and initialize the model object.
	LOG("Creating model object");
	m_Model = new Model;

	result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename1);
	if (!result)
	{
		LOG_ERROR("Could not initialize the model object");
		MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}
	LOG("Model initialized successfully");

	// Create and initialize the floor model
	LOG("Creating floor model");
	m_Floor = new Model;
	strcpy_s(modelFilename, "../Engine/assets/models/Cube.txt");
	strcpy_s(floorTex, "../Engine/assets/textures/Stone02/stone02.tga");
	result = m_Floor->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, floorTex);
	if (!result)
	{
		LOG_ERROR("Could not initialize the floor model");
		MessageBox(hwnd, L"Could not initialize the floor model.", L"Error", MB_OK);
		return false;
	}
	LOG("Floor model initialized successfully");

	// Create and initialize the light object.
	LOG("Creating light object");
	m_Light = new Light;

	// If the model has FBX materials, we'll use those values
	if (m_Model->HasFBXMaterial())
	{
		m_Light->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
		m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularPower(32.0f);
	}
	else
	{
		m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularPower(16.0f);
	}

	m_Light->SetDirection(0.0f, 0.0f, 1.0f);
	LOG("Light initialized successfully");

	// Create and initialize the normal map shader object.
	LOG("Creating shader manager");
	m_ShaderManager = new ShaderManager;

	result = m_ShaderManager->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		LOG_ERROR("Could not initialize shader manager");
		return false;
	}
	LOG("Shader manager initialized successfully");

	// Create and initialize the timer object.
	LOG("Creating timer object");
	m_Timer = new Timer;

	result = m_Timer->Initialize();
	if (!result)
	{
		LOG_ERROR("Could not initialize timer");
		return false;
	}
	LOG("Timer initialized successfully");

	// Create and initialize the model list object.
	LOG("Creating model list");
	m_ModelList = new ModelList;
	m_ModelList->Initialize(6);
	LOG("Model list initialized successfully");

	// Create the position class object.
	LOG("Creating position object");
	m_Position = new Position;

	// Create the frustum class object.
	LOG("Creating frustum object");
	m_Frustum = new Frustum;

	// Create and initialize the user interface object.
	LOG("Creating user interface");
	m_UserInterface = new UserInterface;
	if (!m_UserInterface)
	{
		LOG_ERROR("Could not create user interface");
		return false;
	}

	result = m_UserInterface->Initialize(m_Direct3D, screenHeight, screenWidth);
	if (!result)
	{
		LOG_ERROR("Could not initialize user interface");
		return false;
	}
	LOG("User interface initialized successfully");

	LOG("Application initialization completed successfully");
	return true;
}


void Application::Shutdown()
{
	LOG("Application::Shutdown called");
	// Release the user interface object.
	if (m_UserInterface)
	{
		m_UserInterface->Shutdown();
		delete m_UserInterface;
		m_UserInterface = 0;
	}

	// Release the frustum class object.
	if (m_Frustum)
	{
		delete m_Frustum;
		m_Frustum = 0;
	}

	// Release the position object.
	if (m_Position)
	{
		delete m_Position;
		m_Position = 0;
	}

	// Release the model list object.
	if (m_ModelList)
	{
		m_ModelList->Shutdown();
		delete m_ModelList;
		m_ModelList = 0;
	}

	// Release the timer object.
	if (m_Timer)
	{
		delete m_Timer;
		m_Timer = 0;
	}

	// Release the light object.
	if (m_Light)
	{
		delete m_Light;
		m_Light = 0;
	}

	// Release the model object.
	if (m_Model)
	{
		m_Model->Shutdown();
		delete m_Model;
		m_Model = 0;
	}

	// Release the zone object
	if (m_Zone)
	{
		delete m_Zone;
		m_Zone = 0;
	}

	// Release the camera object.
	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = 0;
	}

	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}

	// Release the floor model
	if (m_Floor)
	{
		m_Floor->Shutdown();
		delete m_Floor;
		m_Floor = 0;
	}

	LOG("Application shutdown completed");
	return;
}


bool Application::Frame(InputManager* Input)
{
	LOG("Application::Frame called");
	float frameTime, rotationY, rotationX;
	float positionX, positionY, positionZ;
	int mouseX, mouseY;
	bool mouseDown, keyDown;
	bool result;

	// Update the system stats.
	LOG("Updating timer");
	m_Timer->Frame();

	// Get the current FPS
	LOG("Getting FPS");
	m_Fps = m_Timer->GetFps();

	// Check if the user pressed escape and wants to exit the application.
	LOG("Checking escape key");
	if (Input->IsEscapePressed())
	{
		LOG("Escape key pressed, exiting application");
		return false;
	}

	// Get the location of the mouse from the input object,
	LOG("Getting mouse location");
	Input->GetMouseLocation(mouseX, mouseY);

	// Get the current frame time.
	LOG("Getting frame time");
	frameTime = m_Timer->GetTime();

	// Update the cursor position
	LOG("Updating cursor position");
	m_UserInterface->UpdateCursorPosition(mouseX, mouseY, frameTime);

	// Check if the mouse has been pressed.
	LOG("Checking mouse state");
	mouseDown = Input->IsMousePressed();

	// Set the frame time for calculating the updated position.
	LOG("Setting frame time for position");
	m_Position->SetFrameTime(frameTime);

	// Get current rotations and position
	LOG("Getting current rotations and position");
	m_Position->GetRotation(rotationY);
	m_Position->GetRotationX(rotationX);
	m_Position->GetPosition(positionX, positionY, positionZ);

	// Handle camera controls based on right mouse button state
	LOG("Handling camera controls");
	if (Input->IsRightMousePressed())
	{
		// When right mouse is pressed, handle rotation based on mouse movement
		int mouseX, mouseY;
		Input->GetMouseLocation(mouseX, mouseY);
		
		// Calculate mouse movement delta
		static int lastMouseX = mouseX;
		static int lastMouseY = mouseY;
		int deltaX = mouseX - lastMouseX;
		int deltaY = mouseY - lastMouseY;
		
		// Update rotations based on mouse movement
		if (deltaX != 0)
		{
			if (deltaX > 0)
				m_Position->LookRight(true);
			else
				m_Position->LookLeft(true);
		}
		if (deltaY != 0)
		{
			if (deltaY > 0)
				m_Position->LookDown(true);
			else
				m_Position->LookUp(true);
		}
		
		// Store current mouse position for next frame
		lastMouseX = mouseX;
		lastMouseY = mouseY;
	}
	else if (Input->IsCtrlPressed())
	{
		// When left control is pressed, only handle movement in y direction
		keyDown = Input->IsUpArrowPressed();
		m_Position->MoveUp(keyDown);

		keyDown = Input->IsDownArrowPressed();
		m_Position->MoveDown(keyDown);
	}
	else
	{
		// When right mouse is not pressed, only handle movement
		keyDown = Input->IsLeftArrowPressed();
		m_Position->MoveLeft(keyDown);

		keyDown = Input->IsRightArrowPressed();
		m_Position->MoveRight(keyDown);

		keyDown = Input->IsUpArrowPressed();
		m_Position->MoveForward(keyDown);

		keyDown = Input->IsDownArrowPressed();
		m_Position->MoveBackward(keyDown);
	}

	// Get the updated position and rotation
	LOG("Getting updated position and rotation");
	m_Position->GetRotation(rotationY);
	m_Position->GetRotationX(rotationX);
	m_Position->GetPosition(positionX, positionY, positionZ);

	// Set the position and rotation of the camera.
	LOG("Setting camera position and rotation");
	m_Camera->SetPosition(positionX, positionY, positionZ);
	m_Camera->SetRotation(rotationX, rotationY, 0.0f);
	m_Camera->Render();

	// Render the graphics scene.
	LOG("Starting render");
	result = Render();
	if (!result)
	{
		LOG_ERROR("Render failed");
		return false;
	}
	LOG("Render completed successfully");

	// Update the user interface.
	LOG("Updating user interface");
	result = m_UserInterface->Frame(m_Direct3D->GetDeviceContext(), m_Fps, m_RenderCount);
	if (!result)
	{
		LOG_ERROR("User interface update failed");
		return false;
	}
	LOG("User interface updated successfully");

	LOG("Frame completed successfully");
	return true;
}


bool Application::Render()
{
	LOG("Application::Render called");
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
	float positionX, positionY, positionZ, radius;
	int modelCount, i;
	bool renderModel;
	bool result;

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);

	// Set render states for skybox
	m_Direct3D->TurnOffCulling();
	m_Direct3D->TurnZBufferOff();

	// Render the zone (skybox)
	LOG("Rendering zone (skybox)");
	result = m_Zone->Render(m_Direct3D, m_ShaderManager, m_Camera);
	if (!result)
	{
		LOG_ERROR("Zone render failed");
		// Restore render states
		m_Direct3D->TurnOnCulling();
		m_Direct3D->TurnZBufferOn();
		return false;
	}

	// Restore render states for the rest of the scene
	m_Direct3D->TurnOnCulling();
	m_Direct3D->TurnZBufferOn();

	// Construct the frustum.
	m_Frustum->ConstructFrustum(viewMatrix, projectionMatrix, SCREEN_DEPTH);

	// Render the floor
	LOG("Rendering floor");
	worldMatrix = XMMatrixTranslation(0.0f, -10.0f, 0.0f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(500.0f, 1.0f, 500.0f));

	m_Floor->Render(m_Direct3D->GetDeviceContext());

	result = m_ShaderManager->RenderLightShader(m_Direct3D->GetDeviceContext(), m_Floor->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
		m_Floor->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(),
		m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
	if (!result)
	{
		LOG_ERROR("Floor render failed");
		return false;
	}

	// Get the number of models that will be rendered.
	modelCount = m_ModelList->GetModelCount();

	// Initialize the count of models that have been rendered.
	m_RenderCount = 0;

	// Go through all the models and render them only if they can be seen by the camera view.
	LOG("Rendering models");
	for (i = 0; i < modelCount; i++)
	{
		// Get the position and color of the sphere model at this index.
		m_ModelList->GetData(i, positionX, positionY, positionZ);

		// Get the model's bounding box
		const Model::AABB& bbox = m_Model->GetBoundingBox();

		// Transform the bounding box to world space
		XMFLOAT3 worldMin, worldMax;
		worldMin.x = bbox.min.x + positionX;
		worldMin.y = bbox.min.y + positionY;
		worldMin.z = bbox.min.z + positionZ;
		worldMax.x = bbox.max.x + positionX;
		worldMax.y = bbox.max.y + positionY;
		worldMax.z = bbox.max.z + positionZ;

		// Check if the model's AABB is in the view frustum
		renderModel = m_Frustum->CheckAABB(worldMin, worldMax);

		// If it can be seen then render it, if not skip this model and check the next one
		if (renderModel)
		{
			// Move the model to the location it should be rendered at.
			worldMatrix = XMMatrixTranslation(positionX, positionY, positionZ);

			// Render the model using the light shader.
			m_Model->Render(m_Direct3D->GetDeviceContext());

			result = m_ShaderManager->RenderLightShader(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Model->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(),
				m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
			if (!result)
			{
				LOG_ERROR("Model render failed");
				return false;
			}

			// Since this model was rendered then increase the count for this frame.
			m_RenderCount++;
		}
	}

	// Create an orthographic projection matrix for 2D rendering
	orthoMatrix = XMMatrixOrthographicLH((float)m_screenWidth, (float)m_screenHeight, 0.0f, 1.0f);

	// Create a fixed view matrix for 2D rendering
	XMMATRIX viewMatrix2D = XMMatrixIdentity();

	// Render the user interface.
	LOG("Rendering user interface");
	result = m_UserInterface->Render(m_Direct3D, m_ShaderManager, worldMatrix, viewMatrix2D, orthoMatrix);
	if (!result)
	{
		LOG_ERROR("User interface render failed");
		return false;
	}

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();
	LOG("Render completed successfully");

	return true;
}

bool Application::Resize(int width, int height)
{
	LOG("Application::Resize called");
	if (width == 0 || height == 0)
	{
		LOG_ERROR("Invalid resize dimensions");
		return false;
	}

	m_screenWidth = width;
	m_screenHeight = height;

	// Resize the Direct3D device
	if (!m_Direct3D->Resize(width, height))
	{
		LOG_ERROR("Direct3D resize failed");
		return false;
	}

	// Update the projection matrix
	float fieldOfView = 3.141592654f / 4.0f;
	float screenAspect = (float)width / (float)height;
	m_Direct3D->GetProjectionMatrix(m_projectionMatrix);

	// Update the orthographic matrix
	m_Direct3D->GetOrthoMatrix(m_orthoMatrix);

	LOG("Resize completed successfully");
	return true;
}