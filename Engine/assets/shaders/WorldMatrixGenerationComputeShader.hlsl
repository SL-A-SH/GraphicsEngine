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

// Helper function to create rotation matrix from Euler angles (in radians) 
// Matches DirectX XMMatrixRotationRollPitchYaw order: Z(roll) * X(pitch) * Y(yaw)
float4x4 CreateRotationMatrix(float3 rotation)
{
    float pitch = rotation.x; // X rotation (pitch)
    float yaw = rotation.y;   // Y rotation (yaw)  
    float roll = rotation.z;  // Z rotation (roll)
    
    float cp = cos(pitch);
    float sp = sin(pitch);
    float cy = cos(yaw);
    float sy = sin(yaw);
    float cr = cos(roll);
    float sr = sin(roll);
    
    // DirectX rotation order: Yaw * Pitch * Roll (Y * X * Z)
    // Row-major matrix to match DirectX conventions
    return float4x4(
        cy * cr + sy * sp * sr,    sr * cp,    -sy * cr + cy * sp * sr,   0.0f,
        -cy * sr + sy * sp * cr,   cr * cp,    sy * sr + cy * sp * cr,    0.0f,
        sy * cp,                   -sp,        cy * cp,                   0.0f,
        0.0f,                      0.0f,       0.0f,                      1.0f
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

// Helper function to create translation matrix (row-major to match DirectX)
float4x4 CreateTranslationMatrix(float3 position)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        position.x, position.y, position.z, 1.0f
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
    
    // Combine matrices to match CPU path: Scale * Rotation * Translation
    // This matches XMMatrixMultiply(XMMatrixMultiply(scaleMatrix, rotationMatrix), translationMatrix)
    float4x4 worldMatrix = mul(mul(scaleMatrix, rotationMatrix), translationMatrix);
    
    // Store the world matrix
    worldMatrixBuffer[objectIndex] = worldMatrix;
} 