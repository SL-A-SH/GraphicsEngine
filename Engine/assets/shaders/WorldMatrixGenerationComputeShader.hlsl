// World Matrix Generation Compute Shader
// Generates world matrices for each object based on position, rotation, and scale

// Constant buffer for object count
cbuffer ObjectCountBuffer : register(b0)
{
    uint objectCount;
    uint padding[3];
};

// Input object data structure
struct ObjectData
{
    float3 position;
    float3 scale;
    float3 rotation;
    float3 boundingBoxMin;
    float3 boundingBoxMax;
    uint objectIndex;
    uint padding[2];
};

// Input buffer
StructuredBuffer<ObjectData> objectBuffer : register(t0);

// Output world matrix buffer
RWStructuredBuffer<float4x4> worldMatrixBuffer : register(u0);

// Helper function to create rotation matrix from Euler angles (in radians) - column-major
float4x4 CreateRotationMatrix(float3 rotation)
{
    float cx = cos(rotation.x);
    float sx = sin(rotation.x);
    float cy = cos(rotation.y);
    float sy = sin(rotation.y);
    float cz = cos(rotation.z);
    float sz = sin(rotation.z);
    
    // Combined rotation matrix (ZYX order) - column-major format
    return float4x4(
        cy * cz,                    cy * sz,                    -sy,        0.0f,
        sx * sy * cz - cx * sz,     sx * sy * sz + cx * cz,     sx * cy,    0.0f,
        cx * sy * cz + sx * sz,     cx * sy * sz - sx * cz,     cx * cy,    0.0f,
        0.0f,                       0.0f,                       0.0f,       1.0f
    );
}

// Helper function to create scale matrix
float4x4 CreateScaleMatrix(float3 scale)
{
    return float4x4(
        scale.x, 0.0f,    0.0f,    0.0f,
        0.0f,    scale.y, 0.0f,    0.0f,
        0.0f,    0.0f,    scale.z, 0.0f,
        0.0f,    0.0f,    0.0f,    1.0f
    );
}

// Helper function to create translation matrix (column-major)
float4x4 CreateTranslationMatrix(float3 position)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, position.x,
        0.0f, 1.0f, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// Thread group size
[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Skip invalid objects
    if (objectIndex >= objectCount)
        return;
    
    // Get object data
    ObjectData object = objectBuffer[objectIndex];
    
    // Create transformation matrices
    float4x4 scaleMatrix = CreateScaleMatrix(object.scale);
    float4x4 rotationMatrix = CreateRotationMatrix(object.rotation);
    float4x4 translationMatrix = CreateTranslationMatrix(object.position);
    
    // Combine matrices: World = Translation * Rotation * Scale
    // For column-major matrices, multiply in standard order
    float4x4 worldMatrix = mul(mul(translationMatrix, rotationMatrix), scaleMatrix);
    
    // Store the world matrix
    worldMatrixBuffer[objectIndex] = worldMatrix;
} 