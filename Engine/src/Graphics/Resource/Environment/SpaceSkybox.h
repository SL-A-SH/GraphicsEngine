#ifndef _SPACESKYBOX_H_
#define _SPACESKYBOX_H_

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>

using namespace DirectX;
using namespace std;

class SpaceSkybox
{
private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    SpaceSkybox();
    SpaceSkybox(const SpaceSkybox&);
    ~SpaceSkybox();

    bool Initialize(ID3D11Device*, ID3D11DeviceContext*);
    void Shutdown();
    void Render(ID3D11DeviceContext*);

    int GetIndexCount();
    float GetTime() const { return m_time; }
    void UpdateTime(float deltaTime);

private:
    bool InitializeBuffers(ID3D11Device*);
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext*);

private:
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    int m_vertexCount;
    int m_indexCount;
    float m_time;
};

#endif 