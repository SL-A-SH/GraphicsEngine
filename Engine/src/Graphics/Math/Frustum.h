#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include <directxmath.h>
using namespace DirectX;

class Frustum
{
public:
    Frustum();
    Frustum(const Frustum&);
    ~Frustum();

    void ConstructFrustum(XMMATRIX, XMMATRIX, float);

    bool CheckPoint(float, float, float);
    bool CheckCube(float, float, float, float);
    bool CheckSphere(float, float, float, float);
    bool CheckRectangle(float, float, float, float, float, float);
    bool CheckAABB(const XMFLOAT3& min, const XMFLOAT3& max);

private:
    XMFLOAT4 m_planes[6];
};

#endif