#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

#include <directxmath.h>
#include <string>

using namespace DirectX;

namespace EngineTypes
{
    // Transform data structure
    struct TransformData
    {
        XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
        XMFLOAT3 rotation = {0.0f, 0.0f, 0.0f};
        XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
    };

    // Bounding box structure
    struct BoundingBox
    {
        XMFLOAT3 min = {0.0f, 0.0f, 0.0f};
        XMFLOAT3 max = {0.0f, 0.0f, 0.0f};
        float radius = 0.0f;  // For backward compatibility with sphere culling
    };

    // Material information structure
    struct MaterialInfo
    {
        std::string diffuseTexturePath;
        std::string normalTexturePath;
        std::string specularTexturePath;
        std::string roughnessTexturePath;
        std::string metallicTexturePath;
        std::string emissionTexturePath;
        std::string aoTexturePath;
        XMFLOAT4 diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        XMFLOAT4 ambientColor = {0.1f, 0.1f, 0.1f, 1.0f};
        XMFLOAT4 specularColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float shininess = 32.0f;
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ao = 1.0f;
        float emissionStrength = 0.0f;
    };

    // Vertex structure for models
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 binormal;
    };

    // Temporary vertex structure for calculations
    struct TempVertexType
    {
        float x, y, z;
        float tu, tv;
        float nx, ny, nz;
    };

    // Vector structure for calculations
    struct VectorType
    {
        float x, y, z;
    };
}

#endif // ENGINE_TYPES_H 