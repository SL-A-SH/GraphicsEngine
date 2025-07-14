// World Matrix Generation Compute Shader
// Generates world matrices for each object based on position, rotation, and scale

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

// Output world matrix structure (4x4 matrix)
// Using float4x4 to match the vertex shader expectation

// Input buffer
StructuredBuffer<ObjectData> objectBuffer : register(t0);

// Output buffer
RWStructuredBuffer<float4x4> worldMatrixBuffer : register(u0);

// Thread group size
[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Check if this thread is processing a valid object
    uint objectCount = 6; // This should be passed as a constant
    
    if (objectIndex >= objectCount)
    {
        return; // Skip invalid objects
    }
    
    // Get object data
    ObjectData object = objectBuffer[objectIndex];
    
    // Generate world matrix from position, rotation, and scale
    float4x4 worldMatrix;
    
    // Convert rotation from radians to degrees and create rotation matrices
    float3 rotationRad = object.rotation;
    
    // Create rotation matrices (simplified - in a real implementation you'd use quaternions)
    float cosX = cos(rotationRad.x);
    float sinX = sin(rotationRad.x);
    float cosY = cos(rotationRad.y);
    float sinY = sin(rotationRad.y);
    float cosZ = cos(rotationRad.z);
    float sinZ = sin(rotationRad.z);
    
    // Rotation around X axis
    float4x4 rotX = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cosX, -sinX, 0.0f,
        0.0f, sinX, cosX, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    // Rotation around Y axis
    float4x4 rotY = float4x4(
        cosY, 0.0f, sinY, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -sinY, 0.0f, cosY, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    // Rotation around Z axis
    float4x4 rotZ = float4x4(
        cosZ, -sinZ, 0.0f, 0.0f,
        sinZ, cosZ, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    // Scale matrix
    float4x4 scaleMatrix = float4x4(
        object.scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, object.scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, object.scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    // Translation matrix
    float4x4 translationMatrix = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        object.position.x, object.position.y, object.position.z, 1.0f
    );
    
    // Combine transformations: scale * rotation * translation
    // This matches the CPU version: scale * rotation * translation
    float4x4 rotationMatrix = mul(mul(rotZ, rotY), rotX);
    float4x4 finalMatrix = mul(mul(scaleMatrix, rotationMatrix), translationMatrix);
    
    // Store the world matrix
    worldMatrix = finalMatrix;
    
    worldMatrixBuffer[objectIndex] = worldMatrix;
} 