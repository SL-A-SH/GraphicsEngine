// GPU-Driven PBR Vertex Shader
// Uses per-instance world matrices from a structured buffer

cbuffer MatrixBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

// World matrices buffer - matches compute shader output format
StructuredBuffer<float4x4> worldMatrixBuffer : register(t1);

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
    
    // Get the world matrix for this instance - now directly a float4x4
    float4x4 worldMatrix = worldMatrixBuffer[instanceID];
    
    // Transpose world matrix to match transposed view/projection matrices
    worldMatrix = transpose(worldMatrix);
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    float4 worldPosition = mul(input.position, worldMatrix);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
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