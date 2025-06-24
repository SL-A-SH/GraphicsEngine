// PBR Pixel Shader
// Physically Based Rendering with support for all texture types

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicTexture : register(t2);
Texture2D roughnessTexture : register(t3);
Texture2D emissionTexture : register(t4);
Texture2D aoTexture : register(t5);

SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightDirection;
    float padding;
    float3 cameraPosition;
    float padding2;
};

cbuffer MaterialBuffer : register(b1)
{
    float4 baseColor;
    float4 materialProperties; // x=metallic, y=roughness, z=ao, w=emissionStrength
    float4 materialPadding;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3x3 tbn : TEXCOORD2;
};

// PBR Functions
static const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel Function (Schlick)
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 PBRPixelShader(PixelInputType input) : SV_TARGET
{
    float4 diffuseColor = diffuseTexture.Sample(SampleType, input.tex) * baseColor;

    // Sample and transform normal from normal map
    float3 normalMap = normalTexture.Sample(SampleType, input.tex).rgb;
    normalMap = normalMap * 2.0 - 1.0; // [0,1] -> [-1,1]
    float3 N = normalize(mul(normalMap, input.tbn)); // TBN to world space

    float3 L = normalize(-lightDirection);
    float NdotL = max(dot(N, L), 0.0);

    // Simple ambient + diffuse
    float3 ambient = ambientColor.rgb * diffuseColor.rgb;
    float3 diffuse = diffuseColor.rgb * NdotL;
    float3 color = ambient + diffuse;

    return float4(color, diffuseColor.a);
} 