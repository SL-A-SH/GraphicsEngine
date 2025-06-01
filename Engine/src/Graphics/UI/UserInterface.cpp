#include "UserInterface.h"

UserInterface::UserInterface()
{
    m_Font = 0;
    m_FpsString = 0;
    m_RenderCountString = 0;
    m_Cursor = 0;
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
    char spriteFilename[128];

    // Create and initialize the font object.
    m_Font = new Font;
    if (!m_Font)
    {
        return false;
    }

    result = m_Font->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), 0);
    if (!result)
    {
        return false;
    }

    // Set the initial fps and fps string.
    m_previousFps = -1;
    strcpy_s(fpsString, "Fps: 0");

    // Create and initialize the text object for the fps string.
    m_FpsString = new Text;
    if (!m_FpsString)
    {
        return false;
    }

    result = m_FpsString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, fpsString, 10, 10, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    // Set the initial render count string.
    strcpy_s(renderString, "Render Count: 0");

    // Create and initialize the text object for the render count string.
    m_RenderCountString = new Text;
    if (!m_RenderCountString)
    {
        return false;
    }

    result = m_RenderCountString->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, renderString, 10, 50, 1.0f, 1.0f, 1.0f);
    if (!result)
    {
        return false;
    }

    // Set the file name of the bitmap file.
    strcpy_s(spriteFilename, "../Engine/assets/sprites/ui/cursor_data.txt");

    // Create and initialize the bitmap object.
    m_Cursor = new Sprite;
    if (!m_Cursor)
    {
        return false;
    }

    result = m_Cursor->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), screenWidth, screenHeight, spriteFilename, 0, 0);
    if (!result)
    {
        return false;
    }

    return true;
}

void UserInterface::Shutdown()
{
    // Release the cursor sprite object.
    if (m_Cursor)
    {
        m_Cursor->Shutdown();
        delete m_Cursor;
        m_Cursor = 0;
    }

    // Release the text objects.
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

    // Put the cursor bitmap vertex and index buffers on the graphics pipeline to prepare them for drawing.
    result = m_Cursor->Render(Direct3D->GetDeviceContext());
    if (!result)
    {
        return false;
    }

    // Render the cursor bitmap with the texture shader.
    result = ShaderManager->RenderTextureShader(Direct3D->GetDeviceContext(), m_Cursor->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_Cursor->GetTexture());
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

void UserInterface::UpdateCursorPosition(int mouseX, int mouseY, float frameTime)
{
    // Update the cursor position with the mouse coordinates
    m_Cursor->SetRenderLocation(mouseX, mouseY);

    // Update the cursor sprite object using the frame time
    m_Cursor->Update(frameTime);
} 