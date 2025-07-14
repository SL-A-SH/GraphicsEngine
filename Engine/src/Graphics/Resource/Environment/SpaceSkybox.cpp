#include "SpaceSkybox.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxpackedvector.h>
#include <fstream>

using namespace DirectX;
using namespace DirectX::PackedVector;

SpaceSkybox::SpaceSkybox()
{
    m_vertexBuffer = 0;
    m_indexBuffer = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_time = 0.0f;
}

SpaceSkybox::SpaceSkybox(const SpaceSkybox& other)
{
}

SpaceSkybox::~SpaceSkybox()
{
}

bool SpaceSkybox::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    bool result;

    // Initialize the vertex and index buffers that hold the geometry for the skybox.
    result = InitializeBuffers(device);
    if (!result)
    {
        return false;
    }

    return true;
}

void SpaceSkybox::Shutdown()
{
    // Release the vertex and index buffers.
    ShutdownBuffers();

    return;
}

void SpaceSkybox::Render(ID3D11DeviceContext* deviceContext)
{
    // Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
    RenderBuffers(deviceContext);

    return;
}

int SpaceSkybox::GetIndexCount()
{
    return m_indexCount;
}

void SpaceSkybox::UpdateTime(float deltaTime)
{
    m_time += deltaTime;
}

bool SpaceSkybox::InitializeBuffers(ID3D11Device* device)
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
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 0;
    indices[4] = 2;
    indices[5] = 3;

    // Back face
    indices[6] = 4;
    indices[7] = 5;
    indices[8] = 6;
    indices[9] = 4;
    indices[10] = 6;
    indices[11] = 7;

    // Top face
    indices[12] = 8;
    indices[13] = 9;
    indices[14] = 10;
    indices[15] = 8;
    indices[16] = 10;
    indices[17] = 11;

    // Bottom face
    indices[18] = 12;
    indices[19] = 13;
    indices[20] = 14;
    indices[21] = 12;
    indices[22] = 14;
    indices[23] = 15;

    // Left face
    indices[24] = 16;
    indices[25] = 17;
    indices[26] = 18;
    indices[27] = 16;
    indices[28] = 18;
    indices[29] = 19;

    // Right face
    indices[30] = 20;
    indices[31] = 21;
    indices[32] = 22;
    indices[33] = 20;
    indices[34] = 22;
    indices[35] = 23;

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

void SpaceSkybox::ShutdownBuffers()
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

void SpaceSkybox::RenderBuffers(ID3D11DeviceContext* deviceContext)
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