#ifndef _SKYBOX_H_
#define _SKYBOX_H_

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>

using namespace DirectX;
using namespace std;

class Skybox
{
private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    Skybox();
    Skybox(const Skybox&);
    ~Skybox();

    bool Initialize(ID3D11Device*, ID3D11DeviceContext*);
    void Shutdown();
    void Render(ID3D11DeviceContext*);

    int GetIndexCount();
    ID3D11ShaderResourceView** GetTextureArray();

private:
    bool InitializeBuffers(ID3D11Device*);
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext*);
    bool LoadTextures(ID3D11Device*, ID3D11DeviceContext*);

private:
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    int m_vertexCount;
    int m_indexCount;
    ID3D11ShaderResourceView* m_textures[6];
};

#endif 