// PBR Vertex Shader
// Supports multiple texture types: diffuse, normal, metallic, roughness, emission, AO
// Also supports GPU-driven rendering with per-instance world matrices

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    uint useGPUDrivenRendering;
    uint padding[3];
};

// World matrices buffer for GPU-driven rendering
StructuredBuffer<float4x4> worldMatrixBuffer : register(t1);

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    uint instanceID : SV_InstanceID;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3x3 tbn : TEXCOORD2; // Tangent-Bitangent-Normal matrix
};

PixelInputType PBRVertexShader(VertexInputType input)
{
    PixelInputType output;
    
    // Choose world matrix based on rendering mode
    float4x4 finalWorldMatrix;
    if (useGPUDrivenRendering != 0)
    {
        // Use per-instance world matrix for GPU-driven rendering
        // Add bounds checking to prevent out-of-bounds access
        uint matrixIndex = input.instanceID;
        if (matrixIndex < 5000) // Max objects limit
        {
            finalWorldMatrix = worldMatrixBuffer[matrixIndex];
        }
        else
        {
            // Fallback to identity matrix if index is out of bounds
            finalWorldMatrix = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
        }
    }
    else
    {
        // Use constant world matrix for CPU-driven rendering
        finalWorldMatrix = worldMatrix;
    }
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    float4 worldPosition = mul(input.position, finalWorldMatrix);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
    // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.normal, (float3x3)finalWorldMatrix);
    output.normal = normalize(output.normal);
    
    // Store the world position for the pixel shader.
    output.worldPos = worldPosition.xyz;
    
    // Calculate the tangent and bitangent vectors against the world matrix.
    float3 tangent = mul(input.tangent, (float3x3)finalWorldMatrix);
    float3 bitangent = mul(input.binormal, (float3x3)finalWorldMatrix);
    
    // Normalize the tangent and bitangent vectors.
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    
    // Create the TBN matrix for normal mapping.
    output.tbn = float3x3(tangent, bitangent, output.normal);
    
    return output;
} 