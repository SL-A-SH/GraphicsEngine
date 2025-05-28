#include "application.h"


Application::Application()
{
	m_Direct3D = 0;
	m_Camera = 0;
	m_Model = 0;
	m_Light = 0;
	m_ShaderManager = 0;
	m_Zone = 0;
	m_Cursor = 0;
	m_Timer = 0;
	m_Font = 0;
	m_FpsString = 0;
	m_RenderCountString = 0;
	m_ModelList = 0;
	m_Position = 0;
	m_Frustum = 0;
	m_Floor = 0;
}


Application::Application(const Application& other)
{
}


Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	char textureFilename1[128], textureFilename2[128], textureFilename3[128];
	char modelFilename[128];
	char floorTex[128];
	char spriteFilename[128];
	char fpsString[32], renderString[32];
	bool result;

	// Create and initialize the Direct3D object.
	m_Direct3D = new D3D11Device;

	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the camera object.
	m_Camera = new Camera;

	// Set the initial position of the camera.
	m_Camera->SetPosition(0.0f, 0.0f, -300.0f);
	m_Camera->Render();
	m_Camera->GetViewMatrix(m_baseViewMatrix);

	// Create and initialize the zone object
	m_Zone = new Zone;

	result = m_Zone->Initialize(m_Direct3D->GetDevice());
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the zone object.", L"Error", MB_OK);
		return false;
	}

	// Set the file name of the model.
	strcpy_s(modelFilename, "../Engine/assets/models/Thriller.fbx");

	// Set the name of the texture file that we will be loading (used as fallback if no FBX material)
	strcpy_s(textureFilename1, "../Engine/assets/textures/Stone02/stone02.tga");
	/*strcpy_s(textureFilename2, "../Engine/assets/textures/Stone02/normal02.tga");
	strcpy_s(textureFilename3, "../Engine/assets/textures/Stone02/spec02.tga");*/

	// Create and initialize the model object.
	m_Model = new Model;

	result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename1);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the floor model
	m_Floor = new Model;
	strcpy_s(modelFilename, "../Engine/assets/models/Cube.txt");
	strcpy_s(floorTex, "../Engine/assets/textures/Stone02/stone02.tga");
	result = m_Floor->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, floorTex);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the floor model.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light object.
	m_Light = new Light;

	// If the model has FBX materials, we'll use those values
	if (m_Model->HasFBXMaterial())
	{
		// Note: You'll need to add getters for these values in ModelClass
		// For now, we'll use default values
		m_Light->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
		m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularPower(32.0f);
	}
	else
	{
		/*m_Light->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);*/
		m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_Light->SetSpecularPower(16.0f);
	}

	m_Light->SetDirection(0.0f, 0.0f, 1.0f);

	// Create and initialize the normal map shader object.
	m_ShaderManager = new ShaderManager;

	result = m_ShaderManager->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		return false;
	}

	// Set the file name of the bitmap file.
	strcpy_s(spriteFilename, "../Engine/assets/sprites/ui/cursor_data.txt");

	// Create and initialize the bitmap object.
	m_Cursor = new Sprite;

	result = m_Cursor->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, spriteFilename, 0, 0);
	if (!result)
	{
		return false;
	}

	// Create and initialize the timer object.
	m_Timer = new Timer;

	result = m_Timer->Initialize();
	if (!result)
	{
		return false;
	}

	// Create and initialize the font object.
	m_Font = new Font;

	result = m_Font->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), 0);
	if (!result)
	{
		return false;
	}

	// Set the initial fps and fps string.
	m_previousFps = -1;
	strcpy_s(fpsString, "Fps: 0");

	// Create and initialize the text object for the fps string.
	m_FpsString = new Text;

	result = m_FpsString->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, fpsString, 10, 0, 0.0f, 1.0f, 0.0f);
	if (!result)
	{
		return false;
	}

	// Set the initial render count string.
	strcpy_s(renderString, "Render Count: 0");

	// Create and initialize the text object for the render count string.
	m_RenderCountString = new Text;

	result = m_RenderCountString->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, renderString, 10, 10, 1.0f, 1.0f, 1.0f);
	if (!result)
	{
		return false;
	}

	// Create and initialize the model list object.
	m_ModelList = new ModelList;
	m_ModelList->Initialize(6);

	// Create the position class object.
	m_Position = new Position;

	// Create the frustum class object.
	m_Frustum = new Frustum;

	return true;
}


void Application::Shutdown()
{
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

	// Release the text objects for the render count string.
	if (m_RenderCountString)
	{
		m_RenderCountString->Shutdown();
		delete m_RenderCountString;
		m_RenderCountString = 0;
	}

	// Release the text object for the fps string.
	if (m_FpsString)
	{
		m_FpsString->Shutdown();
		delete m_FpsString;
		m_FpsString = 0;
	}

	// Release the font object.
	if (m_Font)
	{
		m_Font->Shutdown();
		delete m_Font;
		m_Font = 0;
	}

	// Release the timer object.
	if (m_Timer)
	{
		delete m_Timer;
		m_Timer = 0;
	}

	// Release the cursor sprite object.
	if (m_Cursor)
	{
		m_Cursor->Shutdown();
		delete m_Cursor;
		m_Cursor = 0;
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

	return;
}


bool Application::Frame(InputManager* Input)
{
	float frameTime, rotationY, rotationX;
	float positionX, positionY, positionZ;
	int mouseX, mouseY;
	bool mouseDown, keyDown;
	bool result;

	// Update the system stats.
	m_Timer->Frame();

	// Check if the user pressed escape and wants to exit the application.
	if (Input->IsEscapePressed())
	{
		return false;
	}

	// Get the location of the mouse from the input object,
	Input->GetMouseLocation(mouseX, mouseY);

	// Update the cursor position with the mouse coordinates
	m_Cursor->SetRenderLocation(mouseX, mouseY);

	// Check if the mouse has been pressed.
	mouseDown = Input->IsMousePressed();

	// Get the current frame time.
	frameTime = m_Timer->GetTime();

	// Update the cursor sprite object using the frame time.
	m_Cursor->Update(frameTime);

	// Set the frame time for calculating the updated position.
	m_Position->SetFrameTime(frameTime);

	// Get current rotations and position
	m_Position->GetRotation(rotationY);
	m_Position->GetRotationX(rotationX);
	m_Position->GetPosition(positionX, positionY, positionZ);

	// Handle camera controls based on right mouse button state
	if (Input->IsRightMousePressed())
	{
		// When right mouse is pressed, only handle movement
		keyDown = Input->IsUpArrowPressed();
		m_Position->MoveForward(keyDown);

		keyDown = Input->IsDownArrowPressed();
		m_Position->MoveBackward(keyDown);

		keyDown = Input->IsLeftArrowPressed();
		m_Position->MoveLeft(keyDown);

		keyDown = Input->IsRightArrowPressed();
		m_Position->MoveRight(keyDown);
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
		// When right mouse is not pressed, only handle rotation
		keyDown = Input->IsLeftArrowPressed();
		m_Position->TurnLeft(keyDown);

		keyDown = Input->IsRightArrowPressed();
		m_Position->TurnRight(keyDown);

		keyDown = Input->IsUpArrowPressed();
		m_Position->LookUp(keyDown);

		keyDown = Input->IsDownArrowPressed();
		m_Position->LookDown(keyDown);
	}

	// Get the updated position and rotation
	m_Position->GetRotation(rotationY);
	m_Position->GetRotationX(rotationX);
	m_Position->GetPosition(positionX, positionY, positionZ);

	// Set the position and rotation of the camera.
	m_Camera->SetPosition(positionX, positionY, positionZ);
	m_Camera->SetRotation(rotationX, rotationY, 0.0f);
	m_Camera->Render();

	// Render the graphics scene.
	result = Render();
	if (!result)
	{
		return false;
	}

	// Update the frames per second each frame.
	result = UpdateFps();
	if (!result)
	{
		return false;
	}

	return true;
}


bool Application::Render()
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
	float positionX, positionY, positionZ, radius;
	int modelCount, renderCount, i;
	bool renderModel;
	bool result;

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);

	// Set render states for sky dome
	m_Direct3D->TurnOffCulling();
	m_Direct3D->TurnZBufferOff();

	// Render the zone (sky dome)
	result = m_Zone->Render(m_Direct3D, m_ShaderManager, m_Camera);
	if (!result)
	{
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
	worldMatrix = XMMatrixTranslation(0.0f, -10.0f, 0.0f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(500.0f, 1.0f, 500.0f));

	m_Floor->Render(m_Direct3D->GetDeviceContext());

	result = m_ShaderManager->RenderLightShader(m_Direct3D->GetDeviceContext(), m_Floor->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
		m_Floor->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(),
		m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
	if (!result)
	{
		return false;
	}

	// Get the number of models that will be rendered.
	modelCount = m_ModelList->GetModelCount();

	// Initialize the count of models that have been rendered.
	renderCount = 0;

	// Go through all the models and render them only if they can be seen by the camera view.
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
				return false;
			}

			// Since this model was rendered then increase the count for this frame.
			renderCount++;
		}
	}

	// Update the render count text.
	result = UpdateRenderCountString(renderCount);
	if (!result)
	{
		return false;
	}

	// Disable the Z buffer and enable alpha blending for 2D rendering.
	m_Direct3D->TurnZBufferOff();
	m_Direct3D->EnableAlphaBlending();

	// Reset the world matrix.
	m_Direct3D->GetWorldMatrix(worldMatrix);

	// Render the render count text string using the font shader.
	m_RenderCountString->Render(m_Direct3D->GetDeviceContext());

	result = m_ShaderManager->RenderFontShader(m_Direct3D->GetDeviceContext(), m_RenderCountString->GetIndexCount(), worldMatrix, m_baseViewMatrix, orthoMatrix,
		m_Font->GetTexture(), m_RenderCountString->GetPixelColor());
	if (!result)
	{
		return false;
	}

	// Put the cursor bitmap vertex and index buffers on the graphics pipeline to prepare them for drawing.
	result = m_Cursor->Render(m_Direct3D->GetDeviceContext());
	if (!result)
	{
		return false;
	}

	// Render the cursor bitmap with the texture shader.
	result = m_ShaderManager->RenderTextureShader(m_Direct3D->GetDeviceContext(), m_Cursor->GetIndexCount(), worldMatrix, m_baseViewMatrix, orthoMatrix, m_Cursor->GetTexture());
	if (!result)
	{
		return false;
	}

	// Render the fps text string using the font shader.
	m_FpsString->Render(m_Direct3D->GetDeviceContext());

	result = m_ShaderManager->RenderFontShader(m_Direct3D->GetDeviceContext(), m_FpsString->GetIndexCount(), worldMatrix, m_baseViewMatrix, orthoMatrix,
		m_Font->GetTexture(), m_FpsString->GetPixelColor());
	if (!result)
	{
		return false;
	}

	// Enable the Z buffer and disable alpha blending now that 2D rendering is complete.
	m_Direct3D->TurnZBufferOn();
	m_Direct3D->DisableAlphaBlending();

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	return true;
}

bool Application::UpdateFps()
{
	int fps;
	char tempString[16], finalString[16];
	float red, green, blue;
	bool result;


	// Get the current fps.
	fps = m_Timer->GetFps();

	// Check if the fps from the previous frame was the same, if so don't need to update the text string.
	if (m_previousFps == fps)
	{
		return true;
	}

	// Store the fps for checking next frame.
	m_previousFps = fps;

	// Truncate the fps to below 100,000.
	if (fps > 99999)
	{
		fps = 99999;
	}

	// Convert the fps integer to string format.
	sprintf_s(tempString, "%d", fps);

	// Setup the fps string.
	strcpy_s(finalString, "Fps: ");
	strcat_s(finalString, tempString);

	// If fps is 60 or above set the fps color to green.
	if (fps >= 60)
	{
		red = 0.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 60 set the fps color to yellow.
	if (fps < 60)
	{
		red = 1.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 30 set the fps color to red.
	if (fps < 30)
	{
		red = 1.0f;
		green = 0.0f;
		blue = 0.0f;
	}

	// Update the sentence vertex buffer with the new string information.
	result = m_FpsString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 0, red, green, blue);
	if (!result)
	{
		return false;
	}

	return true;
}


bool Application::UpdateRenderCountString(int renderCount)
{
	char tempString[16], finalString[32];
	bool result;


	// Convert the render count integer to string format.
	sprintf_s(tempString, "%d", renderCount);

	// Setup the render count string.
	strcpy_s(finalString, "Render Count: ");
	strcat_s(finalString, tempString);

	// Update the sentence vertex buffer with the new string information.
	result = m_RenderCountString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 10, 1.0f, 1.0f, 1.0f);
	if (!result)
	{
		return false;
	}

	return true;
}