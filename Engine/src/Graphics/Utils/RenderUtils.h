#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include <d3d11.h>
#include <directxmath.h>
#include <string>

using namespace DirectX;

namespace RenderUtils
{
    // Matrix creation utilities
    XMMATRIX CreateProjectionMatrix(float screenDepth, float screenNear, float fov, float aspectRatio);
    XMMATRIX CreateOrthoMatrix(float screenWidth, float screenHeight, float screenDepth, float screenNear);
    XMMATRIX CreateViewMatrix(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up = {0.0f, 1.0f, 0.0f});
    
    // Vector utilities
    XMFLOAT3 NormalizeVector(const XMFLOAT3& vector);
    XMFLOAT3 CrossProduct(const XMFLOAT3& a, const XMFLOAT3& b);
    float DotProduct(const XMFLOAT3& a, const XMFLOAT3& b);
    
    // Color utilities
    XMFLOAT4 CreateColor(float r, float g, float b, float a = 1.0f);
    XMFLOAT4 LerpColor(const XMFLOAT4& a, const XMFLOAT4& b, float t);
    
    // File utilities
    std::string GetFileExtension(const std::string& filename);
    std::string GetFileNameWithoutExtension(const std::string& filename);
    std::string GetDirectoryPath(const std::string& filepath);
    
    // Math utilities
    float Clamp(float value, float min, float max);
    float Lerp(float a, float b, float t);
    float ToRadians(float degrees);
    float ToDegrees(float radians);
}

#endif // RENDER_UTILS_H 