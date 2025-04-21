
#include "ShaderRegisters.hlsl"

SamplerState DiffuseSampler : register(s0);
SamplerState NormalSampler : register(s1);

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

cbuffer FlagConstants : register(b2)
{
    bool IsLit;
    float3 flagPad0;
}

cbuffer SubMeshConstants : register(b3)
{
    bool IsSelectedSubMesh;
    float3 SubMeshPad0;
}

cbuffer TextureConstants : register(b4)
{
    float2 UVOffset;
    float2 TexturePad0;
}

#include "Light.hlsl"

float4 mainPS(PS_INPUT_StaticMesh Input) : SV_Target
{
    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);

    // Diffuse
    float3 DiffuseColor = Material.DiffuseColor;
    if (Material.TextureFlag & (1 << 1))
    {
        DiffuseColor = DiffuseTexture.Sample(DiffuseSampler, Input.UV).rgb;
        DiffuseColor = SRGBToLinear(DiffuseColor);
    }

    // Normal
    float3 WorldNormal = Input.WorldNormal;
    if (Material.TextureFlag & (1 << 2))
    {
        float3 Normal = NormalTexture.Sample(NormalSampler, Input.UV).rgb;
        Normal = normalize(2.f * Normal - 1.f);
        WorldNormal = normalize(mul(mul(Normal, Input.TBN), (float3x3) InverseTransposedWorld));
    }
    
    // Lighting
    if (IsLit)
    {
        FLightingResult CalculateColor = Lighting(Input.WorldPosition, WorldNormal, Input.WorldViewPosition);
#ifdef LIGHTING_MODEL_GOURAUD
        FinalColor = float4(Input.Color.rgb * DiffuseColor, 1.0);
#elif LIGHTING_MODEL_BLINN_PHONG
        float3 LitColor = (CalculateColor.DiffuseFactor * DiffuseColor) + (Material.SpecularColor * CalculateColor.SpecularFactor);
        FinalColor = float4(LitColor, 1);
#else
        float3 LitColor = Lighting(Input.WorldPosition, WorldNormal, Input.WorldViewPosition).DiffuseFactor;
        FinalColor = float4(LitColor * DiffuseColor, 1);
#endif
    }
    else
    {
        FinalColor = float4(DiffuseColor, 1);
    }
    
    if (bIsSelected)
    {
        FinalColor += float4(0.2, 0.2, 0.2, 1);
    }
    
    return FinalColor;
}
