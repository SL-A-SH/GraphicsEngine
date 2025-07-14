/////////////
// GLOBALS //
/////////////
cbuffer TimeBuffer : register(b0)
{
    float time;
    float mainStarSize;      // Angular size (radians, e.g. 0.05)
    float2 padding0;
    float4 mainStarDir;      // Direction (unit vector, e.g. float3(0,0,1))
    float4 mainStarColor;    // Color (e.g. float3(1,0.95,0.8))
    float mainStarIntensity; // Intensity multiplier (e.g. 1.0)
    float3 padding1;
};

//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
};

// Noise functions for procedural generation
float hash(float n)
{
    return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = p.x + p.y * 57.0 + p.z * 113.0;
    
    // Calculate hash values for all corners
    float h000 = hash(n + 0.0);
    float h001 = hash(n + 1.0);
    float h010 = hash(n + 57.0);
    float h011 = hash(n + 58.0);
    float h100 = hash(n + 113.0);
    float h101 = hash(n + 114.0);
    float h110 = hash(n + 170.0);
    float h111 = hash(n + 171.0);
    
    // Interpolate along x-axis
    float h00 = lerp(h000, h001, f.x);
    float h01 = lerp(h010, h011, f.x);
    float h10 = lerp(h100, h101, f.x);
    float h11 = lerp(h110, h111, f.x);
    
    // Interpolate along y-axis
    float h0 = lerp(h00, h01, f.y);
    float h1 = lerp(h10, h11, f.y);
    
    // Interpolate along z-axis
    return lerp(h0, h1, f.z);
}

float fbm(float3 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value;
}

// Star generation function
float star(float3 p, float3 viewDir)
{
    float3 pos = p * 50.0; // Scale for star density
    float star = noise(pos);
    
    // Create bright stars
    if (star > 0.95)
    {
        // Twinkling effect based on time and view direction
        float twinkle = sin(time * 2.0 + dot(viewDir, pos) * 10.0) * 0.5 + 0.5;
        return twinkle * 2.0;
    }
    
    // Create dimmer stars
    if (star > 0.85)
    {
        return 0.3;
    }
    
    return 0.0;
}

// Nebula generation function
float3 nebula(float3 p)
{
    float3 pos = p * 2.0;
    float n1 = fbm(pos);
    float n2 = fbm(pos + float3(5.2, 1.3, 2.1));
    float n3 = fbm(pos + float3(1.7, 9.2, 1.1));
    
    // Create nebula colors
    float3 nebula1 = float3(0.1, 0.0, 0.3) * n1; // Purple
    float3 nebula2 = float3(0.0, 0.1, 0.2) * n2; // Blue
    float3 nebula3 = float3(0.2, 0.0, 0.1) * n3; // Red
    
    return (nebula1 + nebula2 + nebula3) * 0.3;
}

// Nebula color ramp function
float3 nebulaColorRamp(float t)
{
    // t in [0,1], map to blue-cyan-black (less magenta)
    float3 blue = float3(0.25, 0.45, 0.95);
    float3 cyan = float3(0.5, 0.8, 1.0);
    float3 black = float3(0.0, 0.0, 0.0);
    if (t < 0.5)
        return lerp(blue, cyan, t * 2.0);
    else
        return lerp(cyan, black, (t - 0.5) * 2.0);
}

// Improved FBM for wispy nebula
float fbmNebula(float3 p)
{
    float v = 0.0;
    float a = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 6; i++)
    {
        v += a * noise(p * freq);
        freq *= 2.0;
        a *= 0.55;
    }
    return v;
}

// Star field: many faint, some bright, some color variation
float3 starField(float3 p, float3 viewDir)
{
    float density = 220.0; // Higher = more stars (increased)
    float3 pos = p * density;
    float n = noise(pos);
    float star = smoothstep(0.992, 1.0, n); // Lowered threshold for more stars
    float faint = smoothstep(0.96, 0.992, n) * 0.25; // More faint stars
    float colorShift = frac(dot(pos, float3(12.9898, 78.233, 37.719)));
    float3 color = lerp(float3(1,1,1), float3(0.7,0.8,1.0), colorShift); // Some blueish stars
    float twinkle = 0.7 + 0.3 * sin(time * 2.0 + dot(viewDir, pos) * 10.0);
    return (star * color * twinkle) + (faint * color * 0.5);
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 SpaceSkyboxPixelShader(PixelInputType input) : SV_TARGET
{
    float3 viewDir = normalize(input.viewDir);
    float3 worldPos = normalize(input.worldPos);
    
    // Base space color (very dark blue)
    float3 spaceColor = float3(0.01, 0.02, 0.05);
    
    // Nebula: Layered wispy FBM
    float n1 = fbmNebula(worldPos * 1.5 + 0.2 * time);
    float n2 = fbmNebula(worldPos * 3.0 - 0.1 * time + 10.0);
    float n3 = fbmNebula(worldPos * 6.0 + 5.0);
    float nebulaMask = saturate(n1 * 0.5 + n2 * 0.2 + n3 * 0.1 - 0.35); // Less coverage, more subtle
    float3 nebulaCol = nebulaColorRamp(nebulaMask);
    
    // Nebula alpha for softness
    float nebulaAlpha = pow(saturate(nebulaMask), 1.7) * 0.65; // Less alpha
    
    // Star field
    float3 stars = starField(worldPos, viewDir);
    
    // Main star (soft round glow)
    float angle = acos(dot(viewDir, normalize(mainStarDir.xyz)));

    // Core: sharp, white-blue center
    float core = 1.0 - smoothstep(mainStarSize * 0.2, mainStarSize * 0.5, angle);
    // Glow: soft, blue outer halo
    float glow = 1.0 - smoothstep(mainStarSize * 0.5, mainStarSize * 1.8, angle);
    glow = pow(glow, 1.5);

    float3 coreColor = float3(0.85, 0.95, 1.0); // white-blue
    float3 glowColor = float3(0.3, 0.6, 1.0); // blue

    float3 mainStarCol = core * coreColor * mainStarIntensity + glow * glowColor * (mainStarIntensity * 0.5);
    
    // Combine
    float3 finalColor = lerp(spaceColor, nebulaCol, nebulaAlpha);
    finalColor += stars;
    finalColor = max(finalColor, mainStarCol); // Use max to keep the star on top
    finalColor = saturate(finalColor);
    
    return float4(finalColor, 1.0);
} 