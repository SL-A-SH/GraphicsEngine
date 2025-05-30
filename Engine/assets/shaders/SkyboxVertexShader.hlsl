/////////////
// GLOBALS //
/////////////
cbuffer MatrixBuffer
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType SkyboxVertexShader(VertexInputType input)
{
    PixelInputType output;
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
    // Store the world position for the pixel shader.
    output.worldPos = input.position;
    
    return output;
} 