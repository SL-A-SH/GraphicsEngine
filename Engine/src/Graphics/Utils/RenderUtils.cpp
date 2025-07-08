#include "RenderUtils.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace RenderUtils
{
    XMMATRIX CreateProjectionMatrix(float screenDepth, float screenNear, float fov, float aspectRatio)
    {
        return XMMatrixPerspectiveFovLH(fov, aspectRatio, screenNear, screenDepth);
    }

    XMMATRIX CreateOrthoMatrix(float screenWidth, float screenHeight, float screenDepth, float screenNear)
    {
        return XMMatrixOrthographicLH(screenWidth, screenHeight, screenNear, screenDepth);
    }

    XMMATRIX CreateViewMatrix(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
    {
        XMVECTOR pos = XMLoadFloat3(&position);
        XMVECTOR tgt = XMLoadFloat3(&target);
        XMVECTOR upVec = XMLoadFloat3(&up);
        
        return XMMatrixLookAtLH(pos, tgt, upVec);
    }

    XMFLOAT3 NormalizeVector(const XMFLOAT3& vector)
    {
        XMVECTOR vec = XMLoadFloat3(&vector);
        XMVECTOR normalized = XMVector3Normalize(vec);
        
        XMFLOAT3 result;
        XMStoreFloat3(&result, normalized);
        return result;
    }

    XMFLOAT3 CrossProduct(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMVECTOR vecA = XMLoadFloat3(&a);
        XMVECTOR vecB = XMLoadFloat3(&b);
        XMVECTOR cross = XMVector3Cross(vecA, vecB);
        
        XMFLOAT3 result;
        XMStoreFloat3(&result, cross);
        return result;
    }

    float DotProduct(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMVECTOR vecA = XMLoadFloat3(&a);
        XMVECTOR vecB = XMLoadFloat3(&b);
        XMVECTOR dot = XMVector3Dot(vecA, vecB);
        
        XMFLOAT3 result;
        XMStoreFloat3(&result, dot);
        return result.x;
    }

    XMFLOAT4 CreateColor(float r, float g, float b, float a)
    {
        return {r, g, b, a};
    }

    XMFLOAT4 LerpColor(const XMFLOAT4& a, const XMFLOAT4& b, float t)
    {
        return {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t),
            Lerp(a.w, b.w, t)
        };
    }

    std::string GetFileExtension(const std::string& filename)
    {
        std::filesystem::path path(filename);
        return path.extension().string();
    }

    std::string GetFileNameWithoutExtension(const std::string& filename)
    {
        std::filesystem::path path(filename);
        return path.stem().string();
    }

    std::string GetDirectoryPath(const std::string& filepath)
    {
        std::filesystem::path path(filepath);
        return path.parent_path().string();
    }

    float Clamp(float value, float min, float max)
    {
        return std::clamp(value, min, max);
    }

    float Lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    float ToRadians(float degrees)
    {
        return degrees * XM_PI / 180.0f;
    }

    float ToDegrees(float radians)
    {
        return radians * 180.0f / XM_PI;
    }
} 