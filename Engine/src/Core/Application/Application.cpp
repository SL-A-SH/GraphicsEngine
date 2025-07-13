#include "application.h"
#include "../../Core/System/Logger.h"
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/Common/EngineTypes.h"
#include "../../GUI/Windows/MainWindow.h"
#include "../../GUI/Components/TransformUI.h"
#include "../../GUI/Components/ModelListUI.h"
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Rendering/Camera.h"
#include "../../Graphics/Resource/Model.h"
#include "../../Graphics/Rendering/Light.h"
#include "../../Graphics/Resource/Environment/Zone.h"
#include "../../Core/System/Timer.h"
#include "../../Graphics/Scene/Management/SelectionManager.h"
#include "../../Graphics/Scene/Management/ModelList.h"
#include "../../Graphics/Math/Position.h"
#include "../../Graphics/Math/Frustum.h"
#include "../../Graphics/Rendering/DisplayPlane.h"
#include "../../Graphics/Rendering/GPUDrivenRenderer.h"
#include "../../Graphics/Rendering/IndirectDrawBuffer.h"
#include "../../GUI/Components/UserInterface.h"
#include "../../Core/Input/Management/InputManager.h"
#include "../../Core/System/RenderingBenchmark.h"

Application::Application()
{
	LOG("Application constructor called");
	m_Direct3D = 0;
	m_mainWindow = 0;
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
	m_DisplayPlane = 0;
	m_screenWidth = 0;
	m_screenHeight = 0;
	m_Fps = 0;
	m_RenderCount = 0;
	m_UserInterface = 0;
	
	// New components
	m_SelectionManager = 0;
	m_PositionGizmo = 0;
	m_RotationGizmo = 0;
	m_ScaleGizmo = 0;
	m_GPUDrivenRenderer = 0;
	m_enableGPUDrivenRendering = false;
	m_BenchmarkSystem = 0;
}





Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd, MainWindow* mainWindow)
{
	LOG("Application::Initialize called");
	char textureFilename1[128], textureFilename2[128], textureFilename3[128];
	char modelFilename[128];
	char floorTex[128];
	bool result;

	// Store screen dimensions
	m_mainWindow = mainWindow;
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	// Create and initialize the Direct3D object.
	LOG("Creating Direct3D object");
	m_Direct3D = new D3D11Device;

	result = m_Direct3D->Initialize(screenWidth, screenHeight, AppConfig::VSYNC_ENABLED, hwnd, AppConfig::FULL_SCREEN, AppConfig::SCREEN_DEPTH, AppConfig::SCREEN_NEAR);
	if (!result)
	{
		LOG_ERROR("Could not initialize Direct3D");
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}
	LOG("Direct3D initialized successfully");

	// Initialize the performance profiler
	PerformanceProfiler::GetInstance().Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext());

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
	strcpy_s(modelFilename, "../Engine/assets/models/spaceship/low-poly/nave-modelo.fbx");
	LOG("Attempting to load spaceship model: " + std::string(modelFilename));

	// Create and initialize the model object.
	LOG("Creating model object");
	m_Model = new Model;

	// Use spaceship FBX model with proper FBX initialization
	result = m_Model->InitializeFBX(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename);
	if (!result)
	{
		LOG_ERROR("Could not initialize the model object");
		LOG_ERROR("Model file path: " + std::string(modelFilename));
		MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}
	LOG("Spaceship model initialized successfully");

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

	// Create and initialize gizmo models
	LOG("Creating gizmo models");
	
	// Position gizmo (arrow)
	m_PositionGizmo = new Model;
	strcpy_s(modelFilename, "../Engine/assets/models/Arrow.txt");
	result = m_PositionGizmo->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, nullptr);
	if (!result)
	{
		LOG_ERROR("Could not initialize position gizmo");
		return false;
	}
	
	// Rotation gizmo (arc)
	m_RotationGizmo = new Model;
	strcpy_s(modelFilename, "../Engine/assets/models/Arc.txt");
	result = m_RotationGizmo->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, nullptr);
	if (!result)
	{
		LOG_ERROR("Could not initialize rotation gizmo");
		return false;
	}
	
	// Scale gizmo (line with cube)
	m_ScaleGizmo = new Model;
	strcpy_s(modelFilename, "../Engine/assets/models/ScaleHandle.txt");
	result = m_ScaleGizmo->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, nullptr);
	if (!result)
	{
		LOG_ERROR("Could not initialize scale gizmo");
		return false;
	}
	
	LOG("Gizmo models initialized successfully");

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
	
	// Debug: Check if ModelList was initialized correctly
	int modelCount = m_ModelList->GetModelCount();
	LOG("Application: ModelList reports " + std::to_string(modelCount) + " models after initialization");
	
	if (modelCount <= 0)
	{
		LOG_ERROR("Application: ModelList initialization failed - no models created!");
	}
	else
	{
		LOG("Application: ModelList initialization successful - " + std::to_string(modelCount) + " models created");
	}

	// Create and initialize the selection manager
	LOG("Creating selection manager");
	m_SelectionManager = new SelectionManager;
	result = m_SelectionManager->Initialize(m_Direct3D);
	if (!result)
	{
		LOG_ERROR("Could not initialize selection manager");
		return false;
	}
	LOG("Selection manager initialized successfully");

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
		LOG_ERROR("Could not create User Interface");
		return false;
	}

	result = m_UserInterface->Initialize(m_Direct3D, screenHeight, screenWidth);
	if (!result)
	{
		LOG_ERROR("Could not initialize User Interface");
		return false;
	}
	LOG("User interface initialized successfully");

	// Set up connections between components
	if (m_mainWindow && m_mainWindow->GetTransformUI())
	{
		m_mainWindow->GetTransformUI()->SetSelectionManager(m_SelectionManager);
	}
	if (m_mainWindow && m_mainWindow->GetModelListUI())
	{
		m_mainWindow->GetModelListUI()->SetSelectionManager(m_SelectionManager);
	}

	// Initialize GPU-driven renderer
	m_GPUDrivenRenderer = new GPUDrivenRenderer;
	result = m_GPUDrivenRenderer->Initialize(m_Direct3D->GetDevice(), hwnd, 10000); // Support up to 10,000 objects
	if (!result)
	{
		LOG_ERROR("Could not initialize GPU-driven renderer - will use CPU-driven rendering only");
		// Don't return false - continue with CPU-driven rendering
		m_enableGPUDrivenRendering = false;
	}
	else
	{
		LOG("GPU-driven renderer initialized successfully");
	}

	// Initialize benchmark system
	m_BenchmarkSystem = new RenderingBenchmark;
	result = m_BenchmarkSystem->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd);
	if (!result)
	{
		LOG_ERROR("Could not initialize benchmark system - benchmarking features will be disabled");
		// Don't return false - continue without benchmark system
		delete m_BenchmarkSystem;
		m_BenchmarkSystem = nullptr;
	}
	else
	{
		LOG("Benchmark system initialized successfully");
	}

	// Set up callbacks for model selection
	if (m_mainWindow && m_mainWindow->GetModelListUI())
	{
		// Initialize ModelListUI with Direct3D components
		bool uiInitResult = m_mainWindow->GetModelListUI()->Initialize(m_Direct3D, m_screenHeight, m_screenWidth);
		if (!uiInitResult)
		{
			LOG_ERROR("Failed to initialize ModelListUI with Direct3D components");
		}
		else
		{
			LOG("ModelListUI initialized with Direct3D components successfully");
		}
		
		// Update model list with current models BEFORE setting up callbacks
		LOG("Updating ModelListUI with ModelList data");
		m_mainWindow->GetModelListUI()->UpdateModelList(m_ModelList);
		
		m_mainWindow->GetModelListUI()->SetModelSelectedCallback([this](int modelIndex) {
			LOG("Model selected via UI: " + std::to_string(modelIndex));
			m_SelectionManager->SelectModel(modelIndex);
			// Get the selected model's transform data
			float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;
			m_ModelList->GetTransformData(modelIndex, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ);
			TransformData transformData;
			transformData.position = XMFLOAT3(posX, posY, posZ);
			transformData.rotation = XMFLOAT3(rotX, rotY, rotZ);
			transformData.scale = XMFLOAT3(scaleX, scaleY, scaleZ);
			// Update TransformUI with the selected model's data and switch UI
			if (m_mainWindow && m_mainWindow->GetTransformUI())
			{
				m_mainWindow->GetTransformUI()->SetTransformData(transformData);
				m_mainWindow->SwitchToTransformUI();
			}
			// Call the UI switching callback
			if (m_switchToTransformUICallback)
			{
				m_switchToTransformUICallback();
			}
		});
		m_mainWindow->GetModelListUI()->SetModelDeselectedCallback([this]() {
			LOG("Model deselected via UI");
			m_SelectionManager->DeselectAll();
			if (m_mainWindow && m_mainWindow->GetTransformUI())
			{
				m_mainWindow->GetTransformUI()->ClearTransformData();
			}
			if (m_mainWindow)
			{
				m_mainWindow->SwitchToModelList();
			}
			// Call the UI switching callback
			if (m_switchToModelListCallback)
			{
				m_switchToModelListCallback();
			}
		});
		
		// Show model list UI by default
		m_mainWindow->SwitchToModelList();
	}

	LOG("Model List UI initialized successfully");
	LOG("Transform UI initialized successfully");

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

	// Release gizmo models
	if (m_PositionGizmo)
	{
		m_PositionGizmo->Shutdown();
		delete m_PositionGizmo;
		m_PositionGizmo = 0;
	}
	if (m_RotationGizmo)
	{
		m_RotationGizmo->Shutdown();
		delete m_RotationGizmo;
		m_RotationGizmo = 0;
	}
	if (m_ScaleGizmo)
	{
		m_ScaleGizmo->Shutdown();
		delete m_ScaleGizmo;
		m_ScaleGizmo = 0;
	}

	// Release the GPU-driven renderer
	if (m_GPUDrivenRenderer)
	{
		m_GPUDrivenRenderer->Shutdown();
		delete m_GPUDrivenRenderer;
		m_GPUDrivenRenderer = 0;
	}

	LOG("Application shutdown completed");
	return;
}


bool Application::Frame(InputManager* Input)
{
	// Start profiling the frame
	PerformanceProfiler::GetInstance().BeginFrame();
	
	// Set current rendering mode for profiling
	PerformanceProfiler::GetInstance().SetRenderingMode(static_cast<PerformanceProfiler::RenderingMode>(GetCurrentRenderingMode()));

	float frameTime, rotationY, rotationX;
	float positionX, positionY, positionZ;
	int mouseX, mouseY;
	bool mouseDown, keyDown;
	bool result;

	// Update the system stats.
	m_Timer->Frame();

	// Get the current FPS from PerformanceProfiler instead of Timer for more accurate display
	m_Fps = static_cast<int>(PerformanceProfiler::GetInstance().GetCurrentFPS());

	// Check for F11 fullscreen toggle
	static bool wasF11Pressed = false;
	if (Input->IsF11Pressed() && !wasF11Pressed)
	{
		wasF11Pressed = true;
		// Toggle fullscreen
		m_Direct3D->ToggleFullscreen();
	}
	else if (!Input->IsF11Pressed())
	{
		wasF11Pressed = false;
	}

	// Check for F12 GPU-driven rendering toggle
	static bool wasF12Pressed = false;
	if (Input->IsF12Pressed() && !wasF12Pressed)
	{
		wasF12Pressed = true;
		
		// Check if GPU-driven renderer is properly initialized before toggling
		if (m_GPUDrivenRenderer && m_GPUDrivenRenderer->AreComputeShadersInitialized())
		{
			// Toggle GPU-driven rendering
			m_enableGPUDrivenRendering = !m_enableGPUDrivenRendering;
			m_GPUDrivenRenderer->SetRenderingMode(m_enableGPUDrivenRendering);
			LOG("GPU-driven rendering " + std::string(m_enableGPUDrivenRendering ? "ENABLED" : "DISABLED"));
		}
		else if (!m_GPUDrivenRenderer)
		{
			LOG_ERROR("Cannot toggle GPU-driven rendering - GPUDrivenRenderer is not available (initialization failed)");
		}
		else
		{
			LOG_ERROR("Cannot toggle GPU-driven rendering - compute shaders are not properly initialized");
		}
	}
	else if (!Input->IsF12Pressed())
	{
		wasF12Pressed = false;
	}

	// Get the location of the mouse from the input object
	Input->GetMouseLocation(mouseX, mouseY);

	// Get the current frame time.
	frameTime = m_Timer->GetTime();

	// Check if the mouse has been pressed.
	mouseDown = Input->IsMousePressed();

	// Handle model selection with left mouse click
	static bool wasLeftMousePressed = false;
	if (Input->IsMousePressed() && !wasLeftMousePressed)
	{
		wasLeftMousePressed = true;
		
		LOG("=== MODEL SELECTION DEBUG ===");
		LOG("Mouse click detected at screen coordinates: (" + std::to_string(mouseX) + ", " + std::to_string(mouseY) + ")");
		LOG("Screen dimensions: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
		
		// Convert mouse coordinates to normalized screen coordinates (0-1)
		XMFLOAT2 screenPos;
		screenPos.x = static_cast<float>(mouseX) / static_cast<float>(m_screenWidth);
		screenPos.y = static_cast<float>(mouseY) / static_cast<float>(m_screenHeight);
		
		LOG("Normalized screen position: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + ")");
		
		// Get view and projection matrices for raycasting
		XMMATRIX viewMatrix, projectionMatrix;
		m_Camera->GetViewMatrix(viewMatrix);
		m_Direct3D->GetProjectionMatrix(projectionMatrix);
		
		LOG("Got view and projection matrices");
		
		// Get model instances from ModelList
		std::vector<ModelInstance> modelInstances;
		int modelCount = m_ModelList->GetModelCount();
		LOG("ModelList contains " + std::to_string(modelCount) + " models");
		
		for (int i = 0; i < modelCount; i++)
		{
			float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;
			m_ModelList->GetTransformData(i, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ);
			
			LOG("Model " + std::to_string(i) + " transform from ModelList:");
			LOG("  Position: (" + std::to_string(posX) + ", " + std::to_string(posY) + ", " + std::to_string(posZ) + ")");
			LOG("  Rotation: (" + std::to_string(rotX) + ", " + std::to_string(rotY) + ", " + std::to_string(rotZ) + ")");
			LOG("  Scale: (" + std::to_string(scaleX) + ", " + std::to_string(scaleY) + ", " + std::to_string(scaleZ) + ")");
			
			ModelInstance instance;
			instance.transform.position = XMFLOAT3(posX, posY, posZ);
			instance.transform.rotation = XMFLOAT3(rotX, rotY, rotZ);
			instance.transform.scale = XMFLOAT3(scaleX, scaleY, scaleZ);
			modelInstances.push_back(instance);
		}
		
		LOG("Created " + std::to_string(modelInstances.size()) + " model instances for picking");
		
		// Debug: Print model bounding box information
		if (m_Model)
		{
			const Model::AABB& bbox = m_Model->GetBoundingBox();
			LOG("Model bounding box:");
			LOG("  Min: (" + std::to_string(bbox.min.x) + ", " + std::to_string(bbox.min.y) + ", " + std::to_string(bbox.min.z) + ")");
			LOG("  Max: (" + std::to_string(bbox.max.x) + ", " + std::to_string(bbox.max.y) + ", " + std::to_string(bbox.max.z) + ")");
			LOG("  Radius: " + std::to_string(bbox.radius));
		}
		else
		{
			LOG_ERROR("Model template is null!");
		}
		
		// Perform raycasting to pick a model
		int pickedModel = m_SelectionManager->PickModel(screenPos, viewMatrix, projectionMatrix, 
		                                               modelInstances, m_Model, m_Frustum, m_Camera);
		
		LOG("PickModel returned: " + std::to_string(pickedModel));
		LOG("=== END MODEL SELECTION DEBUG ===");
		
		if (pickedModel >= 0)
		{
			// Model was picked, select it
			LOG("Model " + std::to_string(pickedModel) + " was selected");
			m_SelectionManager->SelectModel(pickedModel);
			
			// Update TransformUI with the selected model's data
			if (m_mainWindow && m_mainWindow->GetTransformUI())
			{
				TransformData transformData = modelInstances[pickedModel].transform;
				m_mainWindow->GetTransformUI()->SetTransformData(transformData);
				LOG("Updated TransformUI with selected model data");
				
				// Switch UI from model list to transform UI
				m_mainWindow->SwitchToTransformUI();
				
				// Call the UI switching callback
				if (m_switchToTransformUICallback)
				{
					m_switchToTransformUICallback();
				}
			}
		}
		else
		{
			// No model was picked, deselect all
			LOG("No model was picked, deselecting all");
			m_SelectionManager->DeselectAll();
			
			// Clear TransformUI and switch back to model list
			if (m_mainWindow && m_mainWindow->GetTransformUI())
			{
				m_mainWindow->GetTransformUI()->ClearTransformData();
				m_mainWindow->GetTransformUI()->HideUI();
				LOG("Cleared TransformUI data");
			}
			
			m_mainWindow->SwitchToModelList();
			
			// Call the UI switching callback
			if (m_switchToModelListCallback)
			{
				m_switchToModelListCallback();
			}
		}
	}
	else if (!Input->IsMousePressed())
	{
		wasLeftMousePressed = false;
	}

	// Set the frame time for calculating the updated position.
	m_Position->SetFrameTime(frameTime);

	// Get current rotations and position
	m_Position->GetRotation(rotationY);
	m_Position->GetRotationX(rotationX);
	m_Position->GetPosition(positionX, positionY, positionZ);

	// Handle camera controls based on right mouse button state
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
		keyDown = Input->IsUpArrowPressed() || Input->IsWPressed();
		m_Position->MoveUp(keyDown);

		keyDown = Input->IsDownArrowPressed() || Input->IsSPressed();
		m_Position->MoveDown(keyDown);
	}
	else
	{
		// When right mouse is not pressed, handle movement with WASD or arrow keys
		keyDown = Input->IsLeftArrowPressed() || Input->IsAPressed();
		m_Position->MoveLeft(keyDown);

		keyDown = Input->IsRightArrowPressed() || Input->IsDPressed();
		m_Position->MoveRight(keyDown);

		keyDown = Input->IsUpArrowPressed() || Input->IsWPressed();
		m_Position->MoveForward(keyDown);

		keyDown = Input->IsDownArrowPressed() || Input->IsSPressed();
		m_Position->MoveBackward(keyDown);
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
		LOG_ERROR("Render failed");
		return false;
	}

	// Update the user interface.
	result = m_UserInterface->Frame(m_Direct3D->GetDeviceContext(), m_Fps, m_RenderCount);
	if (!result)
	{
		LOG_ERROR("User interface update failed");
		return false;
	}

	// Track UI rendering performance
	PerformanceProfiler::GetInstance().IncrementDrawCalls(); // UI rendering adds draw calls
	PerformanceProfiler::GetInstance().AddTriangles(100); // Estimate UI triangle count

	// End profiling the frame
	PerformanceProfiler::GetInstance().EndFrame();

	return true;
}


bool Application::Render()
{
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

	// Get the number of models that will be rendered.
	modelCount = m_ModelList->GetModelCount();

	// Initialize the count of models that have been rendered.
	m_RenderCount = 0;

	// Construct the frustum.
	m_Frustum->ConstructFrustum(viewMatrix, projectionMatrix, AppConfig::SCREEN_DEPTH);

	// Set render states for skybox
	m_Direct3D->TurnOffCulling();
	m_Direct3D->TurnZBufferOff();

	// GPU-Driven Rendering Path
	if (m_enableGPUDrivenRendering && m_GPUDrivenRenderer)
	{
		// Additional safety check to ensure GPU-driven renderer is properly initialized
		if (!m_GPUDrivenRenderer->AreComputeShadersInitialized())
		{
			LOG_ERROR("GPU-driven renderer compute shaders are not properly initialized, falling back to CPU-driven rendering");
			m_enableGPUDrivenRendering = false;
		}
		else if (!m_GPUDrivenRenderer->IsIndirectBufferInitialized())
		{
			LOG_ERROR("GPU-driven renderer indirect buffer is not properly initialized, falling back to CPU-driven rendering");
			m_enableGPUDrivenRendering = false;
		}
		else
		{
			// Render skybox first (CPU-driven)
			m_Direct3D->TurnOffCulling();
			m_Direct3D->TurnZBufferOff();
			result = m_Zone->Render(m_Direct3D, m_ShaderManager, m_Camera);
			if (!result)
			{
				LOG_ERROR("Zone render failed in GPU-driven path");
				m_Direct3D->TurnOnCulling();
				m_Direct3D->TurnZBufferOn();
				return false;
			}
			m_Direct3D->TurnOnCulling();
			m_Direct3D->TurnZBufferOn();
			PerformanceProfiler::GetInstance().IncrementDrawCalls();

			// Render floor (CPU-driven)
			worldMatrix = XMMatrixTranslation(0.0f, -10.0f, 0.0f);
			worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(500.0f, 1.0f, 500.0f));
			m_Floor->Render(m_Direct3D->GetDeviceContext());
			result = m_ShaderManager->RenderLightShader(m_Direct3D->GetDeviceContext(), m_Floor->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Floor->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(),
				m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
			if (!result)
			{
				LOG_ERROR("Floor render failed in GPU-driven path");
				return false;
			}
			PerformanceProfiler::GetInstance().IncrementDrawCalls();
			PerformanceProfiler::GetInstance().AddTriangles(m_Floor->GetIndexCount() / 3);

			// Prepare object data for GPU-driven rendering
			std::vector<ObjectData> objectData;
			objectData.reserve(modelCount);
			
			for (int i = 0; i < modelCount; i++)
			{
				float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;
				m_ModelList->GetTransformData(i, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ);
				
				ObjectData objData;
				objData.position = XMFLOAT3(posX, posY, posZ);
				objData.scale = XMFLOAT3(scaleX, scaleY, scaleZ);
				objData.rotation = XMFLOAT3(rotX, rotY, rotZ);
				objData.boundingBoxMin = XMFLOAT3(-1.0f, -1.0f, -1.0f);
				objData.boundingBoxMax = XMFLOAT3(1.0f, 1.0f, 1.0f);
				objData.objectIndex = static_cast<uint32_t>(i);
				objData.padding[0] = 0;
				objData.padding[1] = 0;
				
				objectData.push_back(objData);
			}
			
			// Update GPU-driven renderer with object data
			m_GPUDrivenRenderer->UpdateObjects(m_Direct3D->GetDeviceContext(), objectData);
			
			// Update camera data
			XMFLOAT3 cameraPos = m_Camera->GetPosition();
			XMFLOAT3 cameraTarget = cameraPos;
			cameraTarget.z += 1.0f; // Look slightly forward
			
			XMMATRIX viewMatrix, projectionMatrix;
			m_Camera->GetViewMatrix(viewMatrix);
			m_Direct3D->GetProjectionMatrix(projectionMatrix);
			
			m_GPUDrivenRenderer->UpdateCamera(m_Direct3D->GetDeviceContext(), cameraPos, cameraTarget, viewMatrix, projectionMatrix);
			
			// Validate that the Model is properly initialized before getting its buffers
			if (!m_Model)
			{
				LOG_ERROR("Application::Render - Model is null, falling back to CPU-driven rendering");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// Check if the Model has valid index count
			int indexCount = m_Model->GetIndexCount();
			LOG("Application::Render - Model statistics:");
			LOG("  IndexCount: " + std::to_string(indexCount));
			
			if (indexCount <= 0)
			{
				LOG_ERROR("Application::Render - Model has no indices, falling back to CPU-driven rendering");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// Check if the Model has been properly initialized (this ensures buffers are created)
			if (!m_Model->GetVertexBuffer() || !m_Model->GetIndexBuffer())
			{
				LOG_ERROR("Application::Render - Model buffers are not initialized, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the Model needs to be rendered at least once to initialize buffers");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// Ensure the Model has been rendered at least once to set up the rendering pipeline
			// This is important because some DirectX operations require the pipeline to be set up
			LOG("Application::Render - Ensuring Model is ready for GPU-driven rendering");
			m_Model->Render(m_Direct3D->GetDeviceContext());
			
			// Validate that the ShaderManager is properly initialized before getting shader resources
			if (!m_ShaderManager)
			{
				LOG_ERROR("Application::Render - ShaderManager is null, falling back to CPU-driven rendering");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// Check if PBR shader files exist (these are required for GPU-driven rendering)
			FILE* testFile;
			errno_t err = fopen_s(&testFile, "../Engine/assets/shaders/PBRVertexShader.hlsl", "r");
			if (err != 0 || !testFile)
			{
				LOG_ERROR("Application::Render - PBRVertexShader.hlsl file not found, falling back to CPU-driven rendering");
				LOG_ERROR("  Expected path: ../Engine/assets/shaders/PBRVertexShader.hlsl");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			fclose(testFile);
			
			err = fopen_s(&testFile, "../Engine/assets/shaders/PBRPixelShader.hlsl", "r");
			if (err != 0 || !testFile)
			{
				LOG_ERROR("Application::Render - PBRPixelShader.hlsl file not found, falling back to CPU-driven rendering");
				LOG_ERROR("  Expected path: ../Engine/assets/shaders/PBRPixelShader.hlsl");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			fclose(testFile);
			
			LOG("Application::Render - PBR shader files found, proceeding with GPU-driven rendering");
			
			// Perform GPU-driven rendering with additional safety checks
			ID3D11Buffer* vertexBuffer = m_Model->GetVertexBuffer();
			ID3D11Buffer* indexBuffer = m_Model->GetIndexBuffer();
			ID3D11VertexShader* vertexShader = m_ShaderManager->GetVertexShader();
			ID3D11PixelShader* pixelShader = m_ShaderManager->GetPixelShader();
			ID3D11InputLayout* inputLayout = m_ShaderManager->GetInputLayout();
			
			// Comprehensive validation of all resources
			LOG("Application::Render - Validating GPU-driven rendering resources:");
			LOG("  VertexBuffer: " + std::string(vertexBuffer ? "valid" : "NULL"));
			LOG("  IndexBuffer: " + std::string(indexBuffer ? "valid" : "NULL"));
			LOG("  VertexShader: " + std::string(vertexShader ? "valid" : "NULL"));
			LOG("  PixelShader: " + std::string(pixelShader ? "valid" : "NULL"));
			LOG("  InputLayout: " + std::string(inputLayout ? "valid" : "NULL"));
			
			// Check if any resource is null
			if (!vertexBuffer)
			{
				LOG_ERROR("Application::Render - VertexBuffer is null, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the Model was not properly initialized or the vertex buffer creation failed");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			if (!indexBuffer)
			{
				LOG_ERROR("Application::Render - IndexBuffer is null, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the Model was not properly initialized or the index buffer creation failed");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			if (!vertexShader)
			{
				LOG_ERROR("Application::Render - VertexShader is null, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the ShaderManager was not properly initialized or the PBR shader failed to load");
				LOG_ERROR("  Check if PBR shader files exist and compile correctly");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			if (!pixelShader)
			{
				LOG_ERROR("Application::Render - PixelShader is null, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the ShaderManager was not properly initialized or the PBR shader failed to load");
				LOG_ERROR("  Check if PBR shader files exist and compile correctly");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			if (!inputLayout)
			{
				LOG_ERROR("Application::Render - InputLayout is null, falling back to CPU-driven rendering");
				LOG_ERROR("  This could indicate the ShaderManager was not properly initialized or the PBR shader failed to load");
				LOG_ERROR("  Check if PBR shader files exist and compile correctly");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// All resources are valid, proceed with GPU-driven rendering
			LOG("Application::Render - All GPU-driven rendering resources are valid, proceeding with render");
			
			// Additional debugging information
			LOG("Application::Render - Debug information:");
			LOG("  Model pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_Model)));
			LOG("  ShaderManager pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_ShaderManager)));
			LOG("  GPUDrivenRenderer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_GPUDrivenRenderer)));
			LOG("  Direct3D pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_Direct3D)));
			LOG("  DeviceContext pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_Direct3D->GetDeviceContext())));
			
			// Test if we can access the buffers safely
			try
			{
				LOG("Application::Render - Testing buffer access:");
				LOG("  VertexBuffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(vertexBuffer)));
				LOG("  IndexBuffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(indexBuffer)));
				LOG("  VertexShader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(vertexShader)));
				LOG("  PixelShader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(pixelShader)));
				LOG("  InputLayout pointer: " + std::to_string(reinterpret_cast<uintptr_t>(inputLayout)));
			}
			catch (...)
			{
				LOG_ERROR("Application::Render - Exception occurred while accessing buffer pointers");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			
			// Call GPU-driven renderer with exception handling
			try
			{
				LOG("Application::Render - Calling GPU-driven renderer...");
				m_GPUDrivenRenderer->Render(m_Direct3D->GetDeviceContext(), vertexBuffer, indexBuffer,
										   vertexShader, pixelShader, inputLayout);
				LOG("Application::Render - GPU-driven renderer call completed successfully");
			}
			catch (const std::exception& e)
			{
				LOG_ERROR("Application::Render - Exception in GPU-driven renderer: " + std::string(e.what()));
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
			catch (...)
			{
				LOG_ERROR("Application::Render - Unknown exception in GPU-driven renderer");
				m_enableGPUDrivenRendering = false;
				return true; // Continue with CPU-driven rendering
			}
		}
	}
	else
	{
		// Traditional CPU-Driven Rendering Path

		// Set render states for skybox
		m_Direct3D->TurnOffCulling();
		m_Direct3D->TurnZBufferOff();

		// Render the zone (skybox)
		result = m_Zone->Render(m_Direct3D, m_ShaderManager, m_Camera);
		if (!result)
		{
			LOG_ERROR("Zone render failed");
			// Restore render states
			m_Direct3D->TurnOnCulling();
			m_Direct3D->TurnZBufferOn();
			return false;
		}

		// Track skybox draw call
		PerformanceProfiler::GetInstance().IncrementDrawCalls();
		// Note: Skybox triangle count would need to be added to Zone class

		// Restore render states for the rest of the scene
		m_Direct3D->TurnOnCulling();
		m_Direct3D->TurnZBufferOn();

		// Render the floor
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

		// Track floor draw call and triangles
		PerformanceProfiler::GetInstance().IncrementDrawCalls();
		PerformanceProfiler::GetInstance().AddTriangles(m_Floor->GetIndexCount() / 3); // Convert indices to triangles

	// Go through all the models and render them only if they can be seen by the camera view.
	for (i = 0; i < modelCount; i++)
	{
		// Get the full transform data for this model
		float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;
		m_ModelList->GetTransformData(i, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ);

		// Get the model's bounding box
		const Model::AABB& bbox = m_Model->GetBoundingBox();

		// Transform the bounding box to world space (considering scale)
		XMFLOAT3 worldMin, worldMax;
		worldMin.x = bbox.min.x * scaleX + posX;
		worldMin.y = bbox.min.y * scaleY + posY;
		worldMin.z = bbox.min.z * scaleZ + posZ;
		worldMax.x = bbox.max.x * scaleX + posX;
		worldMax.y = bbox.max.y * scaleY + posY;
		worldMax.z = bbox.max.z * scaleZ + posZ;

		// Check if the model's AABB is in the view frustum
		renderModel = m_Frustum->CheckAABB(worldMin, worldMax);

		// If it can be seen then render it, if not skip this model and check the next one
		if (renderModel)
		{
			// Create world matrix with position, rotation, and scale
			XMMATRIX translationMatrix = XMMatrixTranslation(posX, posY, posZ);
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ);
			XMMATRIX scaleMatrix = XMMatrixScaling(scaleX, scaleY, scaleZ);
			worldMatrix = XMMatrixMultiply(XMMatrixMultiply(scaleMatrix, rotationMatrix), translationMatrix);

			// Render the model's buffers.
			m_Model->Render(m_Direct3D->GetDeviceContext());

			// Check if this model is selected for visual feedback
			bool isSelected = m_SelectionManager->IsModelSelected(i);
			
			// Check if this is an FBX model with PBR materials first
			if (m_Model->HasFBXMaterial())
			{
				// Debug lighting parameters
				XMFLOAT3 lightDir = m_Light->GetDirection();
				XMFLOAT4 ambientColor = m_Light->GetAmbientColor();
				XMFLOAT4 diffuseColor = m_Light->GetDiffuseColor();
				XMFLOAT3 cameraPos = m_Camera->GetPosition();
				
				// Use PBR shader for FBX models with multiple textures
				result = m_ShaderManager->RenderPBRShader(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
					m_Model->GetDiffuseTexture(), m_Model->GetNormalTexture(), m_Model->GetMetallicTexture(),
					m_Model->GetRoughnessTexture(), m_Model->GetEmissionTexture(), m_Model->GetAOTexture(),
					m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(), m_Model->GetBaseColor(),
					m_Model->GetMetallic(), m_Model->GetRoughness(), m_Model->GetAO(), m_Model->GetEmissionStrength(), m_Camera->GetPosition());
				if (!result)
				{
					LOG_ERROR("Model render with PBRShader failed");
					return false;
				}
			}
			else
			{
				// Get the texture from the model for non-FBX models
				ID3D11ShaderResourceView* modelTexture = m_Model->GetTexture();

				// Only render with the light shader if the model has a texture.
				if (modelTexture)
				{
					// Use regular light shader for simple textured models
					result = m_ShaderManager->RenderLightShader(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
						modelTexture, m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(),
						m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
					if (!result)
					{
						LOG_ERROR("Model render with LightShader failed");
						return false;
					}
				}
				else
				{
					// If there is no texture, render the model with a solid color.
					/*LOG_WARNING("Model has no texture, rendering with ColorShader.");*/
					result = m_ShaderManager->RenderColorShader(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f));
					if (!result)
					{
						LOG_ERROR("Model render with ColorShader failed");
						return false;
					}
				}
			}

			// Track model draw call and triangles
			PerformanceProfiler::GetInstance().IncrementDrawCalls();
			PerformanceProfiler::GetInstance().AddTriangles(m_Model->GetIndexCount() / 3); // Convert indices to triangles
			PerformanceProfiler::GetInstance().AddInstances(1); // Each model instance counts as 1

			// Render selection highlight if this model is selected
			if (isSelected)
			{
				// Render a wireframe outline or different colored version
				// For now, we'll render a simple colored version on top
				m_Direct3D->TurnOffCulling();
				m_Direct3D->TurnZBufferOff();
				
				// Render selection highlight with bright color
				XMFLOAT4 selectionColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.3f); // Yellow with transparency
				result = m_ShaderManager->RenderColorShader(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, selectionColor);
				
				m_Direct3D->TurnOnCulling();
				m_Direct3D->TurnZBufferOn();
				
				if (!result)
				{
					LOG_ERROR("Selection highlight render failed");
				}

				// Track selection highlight draw call
				PerformanceProfiler::GetInstance().IncrementDrawCalls();
			}

			// Since this model was rendered then increase the count for this frame.
			m_RenderCount++;
		}
	}

	// Render gizmos for selected model
	if (m_SelectionManager)
	{
		int selectedIndex = m_SelectionManager->GetSelectedModelIndex();
		if (selectedIndex >= 0 && m_SelectionManager->IsModelSelected(selectedIndex))
		{
			// Get the selected model's full transform data for gizmo placement
			float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;
			m_ModelList->GetTransformData(selectedIndex, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ);
			
			// Create world matrix for gizmo at selected model position with rotation and scale
			XMMATRIX translationMatrix = XMMatrixTranslation(posX, posY, posZ);
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ);
			XMMATRIX scaleMatrix = XMMatrixScaling(scaleX, scaleY, scaleZ);
			XMMATRIX gizmoWorldMatrix = XMMatrixMultiply(XMMatrixMultiply(scaleMatrix, rotationMatrix), translationMatrix);
			
			// Render gizmos
			m_SelectionManager->RenderGizmos(m_Direct3D, viewMatrix, projectionMatrix, gizmoWorldMatrix);

			// Track gizmo draw calls (assuming gizmos add draw calls)
			// This would need to be implemented in SelectionManager::RenderGizmos
		}
	}
	} // End of traditional CPU-driven rendering path

	// Create an orthographic projection matrix for 2D rendering
	orthoMatrix = XMMatrixOrthographicLH((float)m_screenWidth, (float)m_screenHeight, 0.0f, 1.0f);

	// Create a fixed view matrix for 2D rendering
	XMMATRIX viewMatrix2D = XMMatrixIdentity();

	// Render the user interface.
	result = m_UserInterface->Render(m_Direct3D, m_ShaderManager, worldMatrix, viewMatrix2D, orthoMatrix);
	if (!result)
	{
		LOG_ERROR("User interface render failed");
		return false;
	}

	// Track UI draw calls (UI rendering typically adds multiple draw calls)
	// This would need to be implemented in UserInterface::Render

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

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

void Application::SetUISwitchingCallbacks(std::function<void()> switchToModelList, std::function<void()> switchToTransformUI)
{
	m_switchToModelListCallback = switchToModelList;
	m_switchToTransformUICallback = switchToTransformUI;
	LOG("UI switching callbacks set");
}