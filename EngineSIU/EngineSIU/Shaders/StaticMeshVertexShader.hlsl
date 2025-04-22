
#include "ShaderRegisters.hlsl"

#ifdef LIGHTING_MODEL_GOURAUD
SamplerState DiffuseSampler : register(s0);

Texture2D DiffuseTexture : register(t0);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

#include "Light.hlsl"
#endif


PS_INPUT_StaticMesh mainVS(VS_INPUT_StaticMesh Input)
{
    PS_INPUT_StaticMesh Output;

    Output.Position = float4(Input.Position, 1.0);
    Output.Position = mul(Output.Position, WorldMatrix);
    Output.WorldPosition = Output.Position.xyz;
    
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);

    Output.WorldViewPosition = float3(InvViewMatrix._41, InvViewMatrix._42, InvViewMatrix._43);
    
    Output.WorldNormal = mul(Input.Normal, (float3x3)InverseTransposedWorld);

    float3 BiTangent = cross(Input.Normal, Input.Tangent);
    matrix<float, 3, 3> TBN = {
        Input.Tangent.x, Input.Tangent.y, Input.Tangent.z,        // column 0
        BiTangent.x, BiTangent.y, BiTangent.z,                    // column 1
        Input.Normal.x, Input.Normal.y, Input.Normal.z            // column 2
    };
    Output.TBN = TBN;
    
    Output.UV = Input.UV;
    Output.MaterialIndex = Input.MaterialIndex;

#ifdef LIGHTING_MODEL_GOURAUD
    FLightingResult tmp = Lighting(Output.WorldPosition, Output.WorldNormal, ViewWorldLocation);
    Output.Color = float4(tmp.DiffuseFactor, 1.0);
#else
    Output.Color = Input.Color;
#endif
    
    return Output;
}
