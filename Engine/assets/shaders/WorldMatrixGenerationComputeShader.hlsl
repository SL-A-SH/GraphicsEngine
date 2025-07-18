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

// SIMPLIFIED: Only world matrix buffer UAV - no debug buffer
RWStructuredBuffer<float4x4> worldMatrixBuffer : register(u0);

// Thread group size
[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Skip invalid objects
    if (objectIndex >= objectCount)
        return;
        
    // SIMPLE TEST: Write identity matrix for all valid objects
    float4x4 identityMatrix = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    worldMatrixBuffer[objectIndex] = identityMatrix;
} 