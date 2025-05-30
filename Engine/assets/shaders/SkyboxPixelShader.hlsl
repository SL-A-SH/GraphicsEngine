/////////////
// GLOBALS //
/////////////
Texture2D shaderTextures[6] : register(t0);
SamplerState SampleType : register(s0);

//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 SkyboxPixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor;
    float3 absPos = abs(input.worldPos);
    float maxPos = max(max(absPos.x, absPos.y), absPos.z);
    
    // Determine which face of the cube we're rendering
    if (maxPos == absPos.x)
    {
        if (input.worldPos.x > 0)
            textureColor = shaderTextures[0].Sample(SampleType, input.tex); // Right
        else
            textureColor = shaderTextures[1].Sample(SampleType, input.tex); // Left
    }
    else if (maxPos == absPos.y)
    {
        if (input.worldPos.y > 0)
            textureColor = shaderTextures[2].Sample(SampleType, input.tex); // Top
        else
            textureColor = shaderTextures[3].Sample(SampleType, input.tex); // Bottom
    }
    else
    {
        if (input.worldPos.z > 0)
            textureColor = shaderTextures[4].Sample(SampleType, input.tex); // Front
        else
            textureColor = shaderTextures[5].Sample(SampleType, input.tex); // Back
    }
    
    return textureColor;
} 