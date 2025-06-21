#include "UserInterface.h"

UserInterface::UserInterface()
{
    m_screenWidth = 0;
    m_screenHeight = 0;
    m_QTOffset = 200;
    m_Font = 0;
    m_FpsString = 0;
    m_RenderCountString = 0;
    m_previousFps = -1;
    m_previousRenderCount = -1;
    m_ShowFps = false;
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
    int fpsStringX, fpsStringY;
    char renderString[32];
    int renderStringX, renderStringY;

    // Store screen dimensions
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

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

    fpsStringX = m_screenWidth;
    fpsStringY = m_screenHeight - m_QTOffset;
    // Initialize the fps text string with fixed position (20, 20)
    result = m_FpsString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 16, m_Font, fpsString, fpsStringX, fpsStringY, 1.0f, 1.0f, 1.0f);
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

    renderStringX = 0;
    renderStringY = m_screenHeight - m_QTOffset;
    // Initialize the render count text string.
    result = m_RenderCountString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 16, m_Font, renderString, renderStringX, renderStringY, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }


    return true;
}

void UserInterface::Shutdown()
{
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

    // Only render FPS if enabled
    if (m_ShowFps)
    {
        // Render the fps text string using the font shader.
        m_FpsString->Render(Direct3D->GetDeviceContext());

        result = ShaderManager->RenderFontShader(Direct3D->GetDeviceContext(), m_FpsString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
            m_Font->GetTexture(), m_FpsString->GetPixelColor());
        if (!result)
        {
            return false;
        }
    }

    // Render the render count text string using the font shader.
    m_RenderCountString->Render(Direct3D->GetDeviceContext());

    result = ShaderManager->RenderFontShader(Direct3D->GetDeviceContext(), m_RenderCountString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
        m_Font->GetTexture(), m_RenderCountString->GetPixelColor());
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
    // If FPS display is disabled, don't update
    if (!m_ShowFps)
    {
        return true;
    }

    char tempString[16];
    char finalString[16];
    int fpsStringX, fpsStringY;
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

    fpsStringX = m_screenWidth - 400;
    fpsStringY = m_screenHeight - m_QTOffset;
    // Update the sentence vertex buffer with the new string information.
    result = m_FpsString->UpdateText(deviceContext, m_Font, finalString, fpsStringX, fpsStringY, red, green, blue);
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
    int renderStringX, renderStringY;
    bool result;

    // Convert the render count integer to string format.
    sprintf_s(tempString, "%d", renderCount);

    // Setup the render count string.
    strcpy_s(finalString, "Render Count: ");
    strcat_s(finalString, tempString);

    renderStringX = 0;
    renderStringY = m_screenHeight - m_QTOffset;
    // Update the sentence vertex buffer with the new string information.
    result = m_RenderCountString->UpdateText(deviceContext, m_Font, finalString, renderStringX, renderStringY, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    return true;
}