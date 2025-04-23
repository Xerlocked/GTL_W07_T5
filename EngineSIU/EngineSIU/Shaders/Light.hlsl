
#define MAX_LIGHTS 16 

#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16
#define MAX_AMBIENT_LIGHT 16

#define POINT_LIGHT         1
#define SPOT_LIGHT          2
#define DIRECTIONAL_LIGHT   3
#define AMBIENT_LIGHT       4

Texture2D DirectionalShadowMap : register(t2);
Texture2D SpotShadowMap : register(t3);
TextureCubeArray<float2> PointShadowMapArray : register(t4); // ← float 필수!
Texture2D DirectionalShadowCompareMap : register(t5);
Texture2D SpotShadowCompareMap : register(t6);

SamplerState ShadowMapSampler : register(s2);
SamplerComparisonState ShadowMapCompareSampler : register(s3);

struct FAmbientLightInfo
{
    float4 AmbientColor;
};

struct FDirectionalLightInfo
{
    float4 LightColor;

    float3 Direction;
    float Intensity;

    row_major matrix LightViewMatrix;
    row_major matrix LightProjectionMatrix;

    int bCastShadow;
    int Sharpness;
    float2 Pad;
};

struct FPointLightInfo
{
    float4 LightColor;

    float3 Position;
    float AttenuationRadius;

    int Type;
    float Intensity;
    float Falloff;
    int bCastShadow;
    
    row_major matrix LightViewMatrix[6];
    row_major matrix LightProjectionMatrix;

    int Sharpness;
    float3 Pad;
};

struct FSpotLightInfo
{
    float4 LightColor;

    float3 Position;
    float AttenuationRadius;

    float3 Direction;
    float Intensity;

    int Type;
    float InnerRad;
    float OuterRad;
    float Falloff;

    int bCastShadow;
    int Sharpness;
    float2 Pad;

    row_major matrix LightViewMatrix;
    row_major matrix LightProjectionMatrix;
    
    float2 SubUVScale;
    float2 SubUVOffset;
};

struct FLightingResult
{
    float3 DiffuseFactor;
    float3 SpecularFactor;
};
cbuffer Lighting : register(b0)
{
    FAmbientLightInfo Ambient[MAX_AMBIENT_LIGHT];
    FDirectionalLightInfo Directional[MAX_DIRECTIONAL_LIGHT];
    FPointLightInfo PointLights[MAX_POINT_LIGHT];
    FSpotLightInfo SpotLights[MAX_SPOT_LIGHT];
    
    int DirectionalLightsCount;
    int PointLightsCount;
    int SpotLightsCount;
    int AmbientLightsCount;

    float ShadowMapWidth;
    float ShadowMapHeight;
    int FilterMode;
    float Pad;
};

float2 NDCToUV(float3 NDC)
{
    float2 UV = (NDC.xy * 0.5) + 0.5;
    UV.y = 1 - UV.y;
    return UV;
}

bool InRange(float val, float min, float max)
{
    return (min <= val && val <= max);
}

float CalculateShadowByPCF(Texture2D ShadowMap, float2 ShadowMapUV, float Distance)
{
    // 1) 초기화
    float Shadow = 0.0f;
    /////////////////////////////////////////////////////////////
     // PCF
     float OffsetX = 1.f/ShadowMapWidth;
     float OffsetY = 1.f/ShadowMapHeight;
     for (int i=-1;i<=1;i++)
     {
         for (int j=-1;j<=1;j++)
         {
             float2 SampleUV = {
                 ShadowMapUV.x + OffsetX * i,
                 ShadowMapUV.y + OffsetY * j
             };
             if (InRange(SampleUV.x, 0.f, 1.f) && InRange(SampleUV.y, 0.f, 1.f))
             {
                 Shadow += ShadowMap.SampleCmpLevelZero(ShadowMapCompareSampler, SampleUV, Distance).r;
             }else
             {
                 Shadow += 1.f;
             }
         }
     }
     Shadow /= 9;
    return Shadow;
}


float CalculateShadowByVSM(float2 moments, float LightDistance)
{
    float mean = moments.x; //mean depth 평균
    float mean2 = moments.y; //mean2 detph^2 평균
    
    float p = (LightDistance <= mean);
    // Compute variance.
    float Variance = max(mean2 - (mean * mean), 0.00001);
    
    // Compute probabilistic upper bound.
    float d = LightDistance - mean;
    float p_max = Variance / (Variance + d * d);
    float Shadow = max(p, p_max);

    Shadow = pow(Shadow, 2);
    
    return Shadow;
}

float CalculateAttenuation(float Distance, float AttenuationRadius, float Falloff)
{
    if (Distance > AttenuationRadius)
    {
        return 0.0;
    }

    float Result = saturate(1.0f - Distance / AttenuationRadius);
    Result = pow(Result, Falloff);
    
    return Result;
}

float CalculateConeAttenuation(float3 LightDir, float3 SpotDir, float AttenuationRadius, float Falloff, float InnerConeAngle, float OuterConeAngle)
{
    float CosAngle = dot(SpotDir, -LightDir);
    float Outer = cos(radians(OuterConeAngle / 2));
    float Inner = cos(radians(InnerConeAngle / 2));

    float ConeAttenuation = saturate((CosAngle - Outer) / (Inner - Outer));
    ConeAttenuation = pow(ConeAttenuation, Falloff);

    return (CosAngle < 0.0f) ? 0.0f : AttenuationRadius * ConeAttenuation;
}

float CalculateSpotEffect(float3 LightDir, float3 SpotDir, float InnerRadius, float OuterRadius, float SpotFalloff)
{
    float Dot = dot(-LightDir, SpotDir); // [-1,1]
    
    float SpotEffect = smoothstep(cos(OuterRadius / 2), cos(InnerRadius / 2), Dot);
    
    return SpotEffect * pow(max(Dot, 0.0), SpotFalloff);
}

float CalculateDiffuse(float3 WorldNormal, float3 LightDir)
{
    return max(dot(WorldNormal, LightDir), 0.0);
}

float CalculateSpecular(float3 WorldNormal, float3 ToLightDir, float3 ViewDir, float Shininess, float SpecularStrength = 0.5)
{
#ifdef LIGHTING_MODEL_GOURAUD
    float3 ReflectDir = reflect(-ToLightDir, WorldNormal);
    float Spec = pow(max(dot(ViewDir, ReflectDir), 0.0), Shininess);
#else
    float3 HalfDir = normalize(ToLightDir + ViewDir); // Blinn-Phong
    float Spec = pow(max(dot(WorldNormal, HalfDir), 0.0), Shininess);
#endif
    return Spec * SpecularStrength;
}

int GetMajorFaceIndex(float3 dir)
{
    float3 absDir = abs(dir);
    int face = 0;
    if (absDir.x > absDir.y && absDir.x > absDir.z)
        face = dir.x > 0.0 ? 0 : 1;
    else if (absDir.y > absDir.z)
        face = dir.y > 0.0 ? 2 : 3;
    else
        face = dir.z > 0.0 ? 4 : 5;
    return face;
}

FLightingResult PointLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FPointLightInfo LightInfo = PointLights[Index];
    
    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    
    float3 ToLight = LightInfo.Position - WorldPosition;
    float Distance = length(ToLight);

    float3 Dir = normalize(WorldPosition - LightInfo.Position);
    int face = GetMajorFaceIndex(Dir);
    
    float Attenuation = CalculateAttenuation(Distance, LightInfo.AttenuationRadius, LightInfo.Falloff);
    if (Attenuation <= 0.0)
    {
        return Result;
    }
    
    float3 LightDir = normalize(ToLight);
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);
    
    float4 PointLightView = mul(float4(WorldPosition, 1.0), LightInfo.LightViewMatrix[face]);
    float4 PointLightClipPos = mul(PointLightView, LightInfo.LightProjectionMatrix);
    float3 PointShadowMapNDC = PointLightClipPos.xyz / PointLightClipPos.w;
    float2 PointShadowUV = NDCToUV(PointShadowMapNDC);
    float PointLightDistance = PointShadowMapNDC.z;
    
    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);

    float2 moments = PointShadowMapArray.SampleLevel(ShadowMapSampler, float4(Dir, Index), LightInfo.Sharpness).rg;
    
    float Shadow = 1.0f;
    
    if (LightInfo.bCastShadow)
    {
        if (FilterMode == 0)
        {
            Shadow = CalculateShadowByVSM(moments, PointLightDistance);
        }

        if (FilterMode == 1)
        {
            Shadow = CalculateShadowByPCF(SpotShadowMap, PointShadowUV, PointLightDistance);
        }
    }

    Result.DiffuseFactor = DiffuseFactor * LightInfo.LightColor.rgb * Attenuation * LightInfo.Intensity * Shadow;
    Result.SpecularFactor = SpecularFactor * LightInfo.LightColor.rgb * Attenuation * LightInfo.Intensity * Shadow;
    return Result;
}



FLightingResult SpotLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FSpotLightInfo SpotLightInfo = SpotLights[Index];

    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    
    float3 ToLight = SpotLightInfo.Position - WorldPosition;
    float Distance = length(ToLight);
    
    float Attenuation = CalculateAttenuation(Distance, SpotLightInfo.AttenuationRadius, SpotLightInfo.Falloff);

    float3 LightDir = ToLight / Distance;
    float ConeAttenuation = CalculateConeAttenuation(LightDir, normalize(SpotLightInfo.Direction), SpotLightInfo.AttenuationRadius, SpotLightInfo.Falloff, SpotLightInfo.InnerRad, SpotLightInfo.OuterRad);
    
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);

    float4 SpotLightView = mul(float4(WorldPosition, 1.0), SpotLightInfo.LightViewMatrix);
    float4 SpotLightClipPos = mul(SpotLightView, SpotLightInfo.LightProjectionMatrix);
    float3 SpotShadowMapNDC = SpotLightClipPos.xyz / SpotLightClipPos.w;
    float2 SpotShadowMapUV = NDCToUV(SpotShadowMapNDC);
    SpotShadowMapUV = SpotShadowMapUV * SpotLightInfo.SubUVScale + SpotLightInfo.SubUVOffset;

    float SpotLightDistance = SpotShadowMapNDC.z;

    float Shadow = 1.0f;
    float2 moments = SpotShadowMap.SampleLevel(ShadowMapSampler, SpotShadowMapUV, SpotLightInfo.Sharpness).rg;

    if (SpotLightInfo.bCastShadow)
    {
        if (FilterMode == 0)
        {
            Shadow = CalculateShadowByVSM(moments, SpotLightDistance);
        }

        if (FilterMode == 1)
        {
            Shadow = CalculateShadowByPCF(SpotShadowCompareMap, SpotShadowMapUV, SpotLightDistance);
        }
    }
    Result.DiffuseFactor = DiffuseFactor * SpotLightInfo.Intensity * SpotLightInfo.LightColor.rgb * Attenuation * ConeAttenuation * Shadow;

#ifdef LIGHTING_MODEL_LAMBERT
    return Result;
#endif

    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);
    Result.SpecularFactor = SpecularFactor * SpotLightInfo.LightColor.rgb * SpotLightInfo.Intensity * Attenuation * ConeAttenuation * Shadow;
    return Result;
}

FLightingResult DirectionalLight(int nIndex, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FDirectionalLightInfo DirectionalLightInfo = Directional[nIndex];

    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    float3 LightDir = normalize(-DirectionalLightInfo.Direction);
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);
    
    float4 LightView = mul(float4(WorldPosition, 1.0), DirectionalLightInfo.LightViewMatrix);
    float4 LightClipPos = mul(LightView, DirectionalLightInfo.LightProjectionMatrix);
    float3 ShadowMapNDC = LightClipPos.xyz / LightClipPos.w;
    float2 ShadowMapUV = NDCToUV(ShadowMapNDC);
    float LightDistance = ShadowMapNDC.z;
    
    float2 moments = DirectionalShadowMap.SampleLevel(ShadowMapSampler, ShadowMapUV, DirectionalLightInfo.Sharpness).rg;
    float Shadow = 1.0f;

    if (DirectionalLightInfo.bCastShadow)
    {
        if (FilterMode == 0)
        {
            Shadow = CalculateShadowByVSM(moments, LightDistance);
        }

        if (FilterMode == 1)
        {
            Shadow = CalculateShadowByPCF(DirectionalShadowCompareMap, ShadowMapUV, LightDistance);
        }
    }
    
    Result.DiffuseFactor = DiffuseFactor * DirectionalLightInfo.Intensity * DirectionalLightInfo.LightColor.rgb * Shadow;
#ifdef LIGHTING_MODEL_LAMBERT
    return Result;
#endif
    
    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);
    Result.SpecularFactor = SpecularFactor * DirectionalLightInfo.Intensity * DirectionalLightInfo.LightColor.rgb * Shadow;
    return Result;
}


FLightingResult Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FLightingResult Result = (FLightingResult)0;
    
    // 다소 비효율적일 수도 있음.
    [unroll(MAX_POINT_LIGHT)]
    for (int i = 0; i < PointLightsCount; i++)
    {
        FLightingResult tmp = PointLight(i, WorldPosition, WorldNormal, WorldViewPosition);
        Result.DiffuseFactor += tmp.DiffuseFactor;
        Result.SpecularFactor += tmp.SpecularFactor;
        
    }    
    [unroll(MAX_SPOT_LIGHT)]
    for (int j = 0; j < SpotLightsCount; j++)
    {
        FLightingResult tmp = SpotLight(j, WorldPosition, WorldNormal, WorldViewPosition);
        Result.DiffuseFactor += tmp.DiffuseFactor;
        Result.SpecularFactor += tmp.SpecularFactor;
    }
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < DirectionalLightsCount; k++)
    {
        FLightingResult tmp = DirectionalLight(k, WorldPosition, WorldNormal, WorldViewPosition);
        Result.DiffuseFactor += tmp.DiffuseFactor;
        Result.SpecularFactor += tmp.SpecularFactor;
    }
    [unroll(MAX_AMBIENT_LIGHT)]
    for (int l = 0; l < AmbientLightsCount; l++)
    {
        Result.DiffuseFactor += Ambient[l].AmbientColor.rgb * Material.AmbientColor;
    }
    
    return Result;
}
