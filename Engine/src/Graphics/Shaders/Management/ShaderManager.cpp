#include "shadermanager.h"

ShaderManager::ShaderManager()
{
    m_ColorShader = 0;
    m_TextureShader = 0;
    m_LightShader = 0;
    m_NormalMapShader = 0;
    m_SpecMapShader = 0;
    m_FontShader = 0;
    m_SkyboxShader = 0;
    m_PBRShader = 0;
}


ShaderManager::ShaderManager(const ShaderManager& other)
{
}


ShaderManager::~ShaderManager()
{
}


bool ShaderManager::Initialize(ID3D11Device* device, HWND hwnd)
{
    bool result;

    // Create the color shader object.
    m_ColorShader = new ColorShader;
    if (!m_ColorShader)
    {
        return false;
    }

    // Initialize the color shader object.
    result = m_ColorShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }

    // Create and initialize the texture shader object.
    m_TextureShader = new TextureShader;

    result = m_TextureShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }


    // Create and initialize the light shader object.
    m_LightShader = new LightShader;

    result = m_LightShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }


    // Create and initialize the normal map shader object.
    m_NormalMapShader = new NormalMapShader;

    result = m_NormalMapShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }


    // Create and initialize the specular map shader object.
    m_SpecMapShader = new SpecMapShader;

    result = m_SpecMapShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }


    // Create and initialize the font shader object
    m_FontShader = new FontShader;

    result = m_FontShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }

    // Create and initialize the skybox shader object.
    m_SkyboxShader = new SkyboxShader;

    // Initialize the skybox shader object.
    result = m_SkyboxShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }

    // Create and initialize the PBR shader object.
    m_PBRShader = new PBRShader;

    // Initialize the PBR shader object.
    result = m_PBRShader->Initialize(device, hwnd);
    if (!result)
    {
        return false;
    }

    return true;
}


void ShaderManager::Shutdown()
{
    // Release the skybox shader object.
    if (m_SkyboxShader)
    {
        m_SkyboxShader->Shutdown();
        delete m_SkyboxShader;
        m_SkyboxShader = 0;
    }

    // Release the PBR shader object.
    if (m_PBRShader)
    {
        m_PBRShader->Shutdown();
        delete m_PBRShader;
        m_PBRShader = 0;
    }

    // Release the specular map shader object.
    if (m_SpecMapShader)
    {
        m_SpecMapShader->Shutdown();
        delete m_SpecMapShader;
        m_SpecMapShader = 0;
    }

    // Release the normal map shader object.
    if (m_NormalMapShader)
    {
        m_NormalMapShader->Shutdown();
        delete m_NormalMapShader;
        m_NormalMapShader = 0;
    }

    // Release the light shader object.
    if (m_LightShader)
    {
        m_LightShader->Shutdown();
        delete m_LightShader;
        m_LightShader = 0;
    }

    // Release the texture shader object.
    if (m_TextureShader)
    {
        m_TextureShader->Shutdown();
        delete m_TextureShader;
        m_TextureShader = 0;
    }

    return;
}


bool ShaderManager::RenderTextureShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture)
{
    bool result;


    result = m_TextureShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, texture);
    if (!result)
    {
        return false;
    }

    return true;
}


bool ShaderManager::RenderLightShader(ID3D11DeviceContext* deviceContext, int indexCount, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix,
									   const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, const XMFLOAT3& lightDirection,
									   const XMFLOAT4& ambientColor, const XMFLOAT4& diffuseColor, const XMFLOAT3& cameraPosition,
									   const XMFLOAT4& specularColor, float specularPower)
{
	return m_LightShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, texture, lightDirection, ambientColor,
								  diffuseColor, cameraPosition, specularColor, specularPower);
}


bool ShaderManager::RenderNormalMapShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* colorTexture, ID3D11ShaderResourceView* normalTexture, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor)
{
    bool result;


    result = m_NormalMapShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, colorTexture, normalTexture, lightDirection, diffuseColor);
    if (!result)
    {
        return false;
    }

    return true;
}


bool ShaderManager::RenderSpecularMapShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture1, ID3D11ShaderResourceView* texture2, ID3D11ShaderResourceView* texture3, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor, XMFLOAT3 cameraPosition, 
    XMFLOAT4 specularColor, float specularPower)
{
    bool result;

    result = m_SpecMapShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, texture1, texture2, texture3, lightDirection, diffuseColor, cameraPosition, specularColor, specularPower);
    if (!result)
    {
        return false;
    }

    return true;
}

bool ShaderManager::RenderFontShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* texture, 
    XMFLOAT4 pixelColor)
{
    bool result;

    result = m_FontShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, texture, pixelColor);
    if (!result)
    {
        return false;
    }

    return true;
}

bool ShaderManager::RenderSkyboxShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
    XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textures[6])
{
    bool result;

    result = m_SkyboxShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, textures);
    if (!result)
    {
        return false;
    }

    return true;
}

bool ShaderManager::RenderColorShader(ID3D11DeviceContext* deviceContext, int indexCount, const XMMATRIX& worldMatrix,
									   const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMFLOAT4& color)
{
	return m_ColorShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix, color);
}

bool ShaderManager::RenderPBRShader(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
                                   ID3D11ShaderResourceView* diffuseTexture, ID3D11ShaderResourceView* normalTexture, ID3D11ShaderResourceView* metallicTexture,
                                   ID3D11ShaderResourceView* roughnessTexture, ID3D11ShaderResourceView* emissionTexture, ID3D11ShaderResourceView* aoTexture,
                                   XMFLOAT3 lightDirection, XMFLOAT4 ambientColor, XMFLOAT4 diffuseColor, XMFLOAT4 baseColor,
                                   float metallic, float roughness, float ao, float emissionStrength, XMFLOAT3 cameraPosition)
{
    return m_PBRShader->Render(deviceContext, indexCount, worldMatrix, viewMatrix, projectionMatrix,
                              diffuseTexture, normalTexture, metallicTexture, roughnessTexture, emissionTexture, aoTexture,
                              lightDirection, ambientColor, diffuseColor, baseColor, metallic, roughness, ao, emissionStrength, cameraPosition);
}

ID3D11VertexShader* ShaderManager::GetVertexShader() const
{
    // Return the vertex shader from the PBR shader (most complete shader)
    return m_PBRShader ? m_PBRShader->GetVertexShader() : nullptr;
}

ID3D11PixelShader* ShaderManager::GetPixelShader() const
{
    // Return the pixel shader from the PBR shader (most complete shader)
    return m_PBRShader ? m_PBRShader->GetPixelShader() : nullptr;
}

ID3D11InputLayout* ShaderManager::GetInputLayout() const
{
    // Return the input layout from the PBR shader (most complete shader)
    return m_PBRShader ? m_PBRShader->GetInputLayout() : nullptr;
}
