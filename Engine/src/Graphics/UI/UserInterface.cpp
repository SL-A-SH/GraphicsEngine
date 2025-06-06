#include "UserInterface.h"

UserInterface::UserInterface()
{
    m_Font = 0;
    m_FpsString = 0;
    m_RenderCountString = 0;
    m_InputStateString = 0;
    m_previousFps = -1;
    m_previousRenderCount = -1;
}

UserInterface::UserInterface(const UserInterface& other)
{
}

UserInterface::~UserInterface()
{
}

bool UserInterface::Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth)
{
    bool result;
    char fpsString[32];
    char renderString[32];
    char inputString[128];

    // Create the font object.
    m_Font = new Font;
    if (!m_Font)
    {
        return false;
    }

    // Initialize the font object with default font (choice 0)
    result = m_Font->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), 0);
    if (!result)
    {
        return false;
    }

    strcpy_s(fpsString, "Fps: 0");

    // Create the text object for the fps string.
    m_FpsString = new Text;
    if (!m_FpsString)
    {
        return false;
    }

    // Initialize the fps text string.
    result = m_FpsString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 16, m_Font, fpsString, 10, 10, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    strcpy_s(renderString, "Render Count: 0");

    // Create the text object for the render count string.
    m_RenderCountString = new Text;
    if (!m_RenderCountString)
    {
        return false;
    }

    // Initialize the render count text string.
    result = m_RenderCountString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 16, m_Font, renderString, 10, 50, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    // Initialize the input state text string
    strcpy_s(inputString, "Input: RMouse[ ] W[ ] A[ ] S[ ] D[ ] F11[ ]");

    // Create the text object for input state
    m_InputStateString = new Text;
    if (!m_InputStateString)
    {
        return false;
    }

    result = m_InputStateString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 64, m_Font, inputString, 10, 90, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    return true;
}

void UserInterface::Shutdown()
{
    // Release the text objects.
    if (m_InputStateString)
    {
        m_InputStateString->Shutdown();
        delete m_InputStateString;
        m_InputStateString = 0;
    }

    if (m_RenderCountString)
    {
        m_RenderCountString->Shutdown();
        delete m_RenderCountString;
        m_RenderCountString = 0;
    }

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

    return;
}

bool UserInterface::Frame(ID3D11DeviceContext* deviceContext, int fps, int renderCount)
{
    bool result;

    // Update the fps string.
    result = UpdateFpsString(deviceContext, fps);
    if (!result)
    {
        return false;
    }

    // Update the render count string.
    result = UpdateRenderCountString(deviceContext, renderCount);
    if (!result)
    {
        return false;
    }

    return true;
}

bool UserInterface::Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix)
{
    bool result;

    // Turn off the Z buffer and enable alpha blending for 2D rendering.
    Direct3D->TurnZBufferOff();
    Direct3D->EnableAlphaBlending();

    // Reset the world matrix to identity for 2D rendering
    worldMatrix = XMMatrixIdentity();

    // Render the fps text string using the font shader.
    m_FpsString->Render(Direct3D->GetDeviceContext());

    result = ShaderManager->RenderFontShader(Direct3D->GetDeviceContext(), m_FpsString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
        m_Font->GetTexture(), m_FpsString->GetPixelColor());
    if (!result)
    {
        return false;
    }

    // Render the render count text string using the font shader.
    m_RenderCountString->Render(Direct3D->GetDeviceContext());

    result = ShaderManager->RenderFontShader(Direct3D->GetDeviceContext(), m_RenderCountString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
        m_Font->GetTexture(), m_RenderCountString->GetPixelColor());
    if (!result)
    {
        return false;
    }

    // Render the input state text string
    m_InputStateString->Render(Direct3D->GetDeviceContext());

    result = ShaderManager->RenderFontShader(Direct3D->GetDeviceContext(), m_InputStateString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
        m_Font->GetTexture(), m_InputStateString->GetPixelColor());
    if (!result)
    {
        return false;
    }

    // Turn off alpha blending now that the text has been rendered.
    Direct3D->DisableAlphaBlending();

    // Turn the Z buffer back on now that the 2D rendering has completed.
    Direct3D->TurnZBufferOn();

    return true;
}

bool UserInterface::UpdateFpsString(ID3D11DeviceContext* deviceContext, int fps)
{
    char tempString[16];
    char finalString[16];
    float red, green, blue;
    bool result;

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
    result = m_FpsString->UpdateText(deviceContext, m_Font, finalString, 10, 10, red, green, blue);
    if (!result)
    {
        return false;
    }

    return true;
}

bool UserInterface::UpdateRenderCountString(ID3D11DeviceContext* deviceContext, int renderCount)
{
    char tempString[16];
    char finalString[32];
    bool result;

    // Convert the render count integer to string format.
    sprintf_s(tempString, "%d", renderCount);

    // Setup the render count string.
    strcpy_s(finalString, "Render Count: ");
    strcat_s(finalString, tempString);

    // Update the sentence vertex buffer with the new string information.
    result = m_RenderCountString->UpdateText(deviceContext, m_Font, finalString, 10, 50, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    return true;
}

bool UserInterface::UpdateInputState(ID3D11DeviceContext* deviceContext, bool rightMousePressed, bool wPressed, bool aPressed, bool sPressed, bool dPressed, bool f11Pressed)
{
    char inputString[128];
    bool result;

    // Create the input state string
    sprintf_s(inputString, "Input: RMouse[%c] W[%c] A[%c] S[%c] D[%c] F11[%c]",
        rightMousePressed ? 'X' : ' ',
        wPressed ? 'X' : ' ',
        aPressed ? 'X' : ' ',
        sPressed ? 'X' : ' ',
        dPressed ? 'X' : ' ',
        f11Pressed ? 'X' : ' ');

    // Update the sentence vertex buffer with the new string information.
    result = m_InputStateString->UpdateText(deviceContext, m_Font, inputString, 10, 90, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    return true;
} 