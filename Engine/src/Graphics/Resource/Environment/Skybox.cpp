#include "skybox.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxpackedvector.h>
#include <fstream>
#include "../Texture.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

Skybox::Skybox()
{
    m_vertexBuffer = 0;
    m_indexBuffer = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    for (int i = 0; i < 6; i++)
    {
        m_textures[i] = 0;
    }
}

Skybox::Skybox(const Skybox& other)
{
}

Skybox::~Skybox()
{
}

bool Skybox::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    bool result;

    // Initialize the vertex and index buffers that hold the geometry for the skybox.
    result = InitializeBuffers(device);
    if (!result)
    {
        return false;
    }

    // Load the textures for the skybox.
    result = LoadTextures(device, deviceContext);
    if (!result)
    {
        return false;
    }

    return true;
}

void Skybox::Shutdown()
{
    // Release the textures.
    for (int i = 0; i < 6; i++)
    {
        if (m_textures[i])
        {
            m_textures[i]->Release();
            m_textures[i] = 0;
        }
    }

    // Release the vertex and index buffers.
    ShutdownBuffers();

    return;
}

void Skybox::Render(ID3D11DeviceContext* deviceContext)
{
    // Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
    RenderBuffers(deviceContext);

    return;
}

int Skybox::GetIndexCount()
{
    return m_indexCount;
}

ID3D11ShaderResourceView** Skybox::GetTextureArray()
{
    return m_textures;
}

bool Skybox::InitializeBuffers(ID3D11Device* device)
{
    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;

    // Set the number of vertices in the vertex array.
    m_vertexCount = 24;  // 4 vertices per face * 6 faces

    // Set the number of indices in the index array.
    m_indexCount = 36;  // 6 indices per face * 6 faces

    // Create the vertex array.
    vertices = new VertexType[m_vertexCount];
    if (!vertices)
    {
        return false;
    }

    // Create the index array.
    indices = new unsigned long[m_indexCount];
    if (!indices)
    {
        return false;
    }

    // Load the vertex array with data.
    // Front face
    vertices[0].position = XMFLOAT3(-1.0f, 1.0f, -1.0f);  // Top left
    vertices[0].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[1].position = XMFLOAT3(-1.0f, -1.0f, -1.0f); // Bottom left
    vertices[1].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[2].position = XMFLOAT3(1.0f, -1.0f, -1.0f);  // Bottom right
    vertices[2].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[3].position = XMFLOAT3(1.0f, 1.0f, -1.0f);   // Top right
    vertices[3].texture = XMFLOAT2(1.0f, 0.0f);

    // Back face
    vertices[4].position = XMFLOAT3(1.0f, 1.0f, 1.0f);    // Top left
    vertices[4].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[5].position = XMFLOAT3(1.0f, -1.0f, 1.0f);   // Bottom left
    vertices[5].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[6].position = XMFLOAT3(-1.0f, -1.0f, 1.0f);  // Bottom right
    vertices[6].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[7].position = XMFLOAT3(-1.0f, 1.0f, 1.0f);   // Top right
    vertices[7].texture = XMFLOAT2(1.0f, 0.0f);

    // Top face
    vertices[8].position = XMFLOAT3(-1.0f, 1.0f, 1.0f);   // Top left
    vertices[8].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[9].position = XMFLOAT3(-1.0f, 1.0f, -1.0f);  // Bottom left
    vertices[9].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[10].position = XMFLOAT3(1.0f, 1.0f, -1.0f);  // Bottom right
    vertices[10].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[11].position = XMFLOAT3(1.0f, 1.0f, 1.0f);   // Top right
    vertices[11].texture = XMFLOAT2(1.0f, 0.0f);

    // Bottom face
    vertices[12].position = XMFLOAT3(-1.0f, -1.0f, -1.0f); // Top left
    vertices[12].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[13].position = XMFLOAT3(-1.0f, -1.0f, 1.0f);  // Bottom left
    vertices[13].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[14].position = XMFLOAT3(1.0f, -1.0f, 1.0f);   // Bottom right
    vertices[14].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[15].position = XMFLOAT3(1.0f, -1.0f, -1.0f);  // Top right
    vertices[15].texture = XMFLOAT2(1.0f, 0.0f);

    // Left face
    vertices[16].position = XMFLOAT3(-1.0f, 1.0f, 1.0f);   // Top left
    vertices[16].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[17].position = XMFLOAT3(-1.0f, -1.0f, 1.0f);  // Bottom left
    vertices[17].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[18].position = XMFLOAT3(-1.0f, -1.0f, -1.0f); // Bottom right
    vertices[18].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[19].position = XMFLOAT3(-1.0f, 1.0f, -1.0f);  // Top right
    vertices[19].texture = XMFLOAT2(1.0f, 0.0f);

    // Right face
    vertices[20].position = XMFLOAT3(1.0f, 1.0f, -1.0f);   // Top left
    vertices[20].texture = XMFLOAT2(0.0f, 0.0f);
    vertices[21].position = XMFLOAT3(1.0f, -1.0f, -1.0f);  // Bottom left
    vertices[21].texture = XMFLOAT2(0.0f, 1.0f);
    vertices[22].position = XMFLOAT3(1.0f, -1.0f, 1.0f);   // Bottom right
    vertices[22].texture = XMFLOAT2(1.0f, 1.0f);
    vertices[23].position = XMFLOAT3(1.0f, 1.0f, 1.0f);    // Top right
    vertices[23].texture = XMFLOAT2(1.0f, 0.0f);

    // Load the index array with data.
    // Front face
    indices[0] = 0;  // Top left
    indices[1] = 1;  // Bottom left
    indices[2] = 2;  // Bottom right
    indices[3] = 0;  // Top left
    indices[4] = 2;  // Bottom right
    indices[5] = 3;  // Top right

    // Back face
    indices[6] = 4;   // Top left
    indices[7] = 5;   // Bottom left
    indices[8] = 6;   // Bottom right
    indices[9] = 4;   // Top left
    indices[10] = 6;  // Bottom right
    indices[11] = 7;  // Top right

    // Top face
    indices[12] = 8;   // Top left
    indices[13] = 9;   // Bottom left
    indices[14] = 10;  // Bottom right
    indices[15] = 8;   // Top left
    indices[16] = 10;  // Bottom right
    indices[17] = 11;  // Top right

    // Bottom face
    indices[18] = 12;  // Top left
    indices[19] = 13;  // Bottom left
    indices[20] = 14;  // Bottom right
    indices[21] = 12;  // Top left
    indices[22] = 14;  // Bottom right
    indices[23] = 15;  // Top right

    // Left face
    indices[24] = 16;  // Top left
    indices[25] = 17;  // Bottom left
    indices[26] = 18;  // Bottom right
    indices[27] = 16;  // Top left
    indices[28] = 18;  // Bottom right
    indices[29] = 19;  // Top right

    // Right face
    indices[30] = 20;  // Top left
    indices[31] = 21;  // Bottom left
    indices[32] = 22;  // Bottom right
    indices[33] = 20;  // Top left
    indices[34] = 22;  // Bottom right
    indices[35] = 23;  // Top right

    // Set up the description of the static vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Now create the vertex buffer.
    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Set up the description of the static index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Release the arrays now that the vertex and index buffers have been created and loaded.
    delete[] vertices;
    vertices = 0;

    delete[] indices;
    indices = 0;

    return true;
}

void Skybox::ShutdownBuffers()
{
    // Release the index buffer.
    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = 0;
    }

    // Release the vertex buffer.
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = 0;
    }

    return;
}

void Skybox::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
    unsigned int stride;
    unsigned int offset;

    // Set vertex buffer stride and offset.
    stride = sizeof(VertexType);
    offset = 0;

    // Set the vertex buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return;
}

bool Skybox::LoadTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    HRESULT result;
    char textureFilename[128];
    int error;
    Texture* texture;

    // Load the six textures for the skybox.
    for (int i = 0; i < 6; i++)
    {
        // Set the texture filename based on the face.
        switch (i)
        {
        case 0: // Right face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/right.png");
            break;
        case 1: // Left face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/left.png");
            break;
        case 2: // Top face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/top.png");
            break;
        case 3: // Bottom face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/bottom.png");
            break;
        case 4: // Front face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/front.png");
            break;
        case 5: // Back face
            error = strcpy_s(textureFilename, 128, "../Engine/assets/textures/skybox/back.png");
            break;
        default:
            return false;
        }

        if (error != 0)
        {
            return false;
        }

        // Create and initialize the texture object
        texture = new Texture;
        result = texture->Initialize(device, deviceContext, textureFilename);
        if (FAILED(result))
        {
            delete texture;
            return false;
        }

        // Get the shader resource view
        m_textures[i] = texture->GetTexture();
        delete texture;
    }

    return true;
} 