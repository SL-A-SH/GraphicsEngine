// GPU-Driven PBR Vertex Shader
// Uses per-instance world matrices from a structured buffer

// World matrix structure (4x4 matrix as 4 float4s)
struct WorldMatrix
{
    float4 row0;
    float4 row1;
    float4 row2;
    float4 row3;
};

cbuffer MatrixBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

// World matrices buffer
StructuredBuffer<WorldMatrix> worldMatrixBuffer : register(t1);

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

PixelInputType GPUDrivenPBRVertexShader(VertexInputType input)
{
    PixelInputType output;
    
    // Get the world matrix for this instance
    WorldMatrix worldMatrix = worldMatrixBuffer[input.instanceID];
    
    // Reconstruct the 4x4 matrix from the 4 float4s
    matrix worldMatrix4x4 = matrix(
        worldMatrix.row0,
        worldMatrix.row1,
        worldMatrix.row2,
        worldMatrix.row3
    );
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    float4 worldPosition = mul(input.position, worldMatrix4x4);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
    // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.normal, (float3x3)worldMatrix4x4);
    output.normal = normalize(output.normal);
    
    // Store the world position for the pixel shader.
    output.worldPos = worldPosition.xyz;
    
    // Calculate the tangent and bitangent vectors against the world matrix.
    float3 tangent = mul(input.tangent, (float3x3)worldMatrix4x4);
    float3 bitangent = mul(input.binormal, (float3x3)worldMatrix4x4);
    
    // Normalize the tangent and bitangent vectors.
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    
    // Create the TBN matrix for normal mapping.
    output.tbn = float3x3(tangent, bitangent, output.normal);
    
    return output;
} 