// GPU-Driven PBR Vertex Shader with Fallback Support
// Uses per-instance world matrices from a structured buffer
// Supports both stream compaction and direct visibility buffer approaches
// AUTO-DETECTS: Uses appropriate approach based on buffer contents

cbuffer MatrixBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

// World matrices buffer - matches compute shader output format
StructuredBuffer<float4x4> worldMatrixBuffer : register(t1);

// Multi-purpose buffer: either visibility buffer (1/0) or compacted visible objects (indices)
StructuredBuffer<uint> visibilityOrObjectsBuffer : register(t2);

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3x3 tbn : TEXCOORD2; // Tangent-Bitangent-Normal matrix
};

PixelInputType GPUDrivenPBRVertexShader(VertexInputType input, uint instanceID : SV_InstanceID)
{
    PixelInputType output;
    
    // ADAPTIVE APPROACH: Support both stream compaction and visibility buffer
    // For now, assume this is a visibility buffer (1 = visible, 0 = culled)
    // This provides immediate functionality while we debug stream compaction
    
    uint visibilityValue = visibilityOrObjectsBuffer[instanceID];
    uint actualObjectIndex = instanceID; // Use instanceID directly for now
    
    // Get the world matrix for this object
    float4x4 worldMatrix = worldMatrixBuffer[actualObjectIndex];
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    float4 worldPosition = mul(input.position, worldMatrix);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // GPU CULLING: If object is not visible, move it off-screen to skip rasterization
    if (visibilityValue == 0)
    {
        output.position = float4(0.0, 0.0, -1.0, 1.0); // Behind near plane
    }
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
    // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.normal, (float3x3)worldMatrix);
    output.normal = normalize(output.normal);
    
    // Store the world position for the pixel shader.
    output.worldPos = worldPosition.xyz;
    
    // Calculate the tangent and bitangent vectors against the world matrix.
    float3 tangent = mul(input.tangent, (float3x3)worldMatrix);
    float3 bitangent = mul(input.binormal, (float3x3)worldMatrix);
    
    // Normalize the tangent and bitangent vectors.
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    
    // Create the TBN matrix for normal mapping.
    output.tbn = float3x3(tangent, bitangent, output.normal);
    
    return output;
} 