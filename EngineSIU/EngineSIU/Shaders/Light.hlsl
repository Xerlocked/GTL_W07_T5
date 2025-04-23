
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
TextureCubeArray<float> PointShadowMapArray : register(t4); // ← float 필수!


SamplerComparisonState ShadowMapSampler : register(s2);

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
};

struct FPointLightInfo
{
    float4 LightColor;

    float3 Position;
    float AttenuationRadius;

    int Type;
    float Intensity;
    float Falloff;
    float Padding;
    
    row_major matrix LightViewMatrix[6];
    row_major matrix LightProjectionMatrix;
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

    row_major matrix LightViewMatrix;
    row_major matrix LightProjectionMatrix;
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
    float2 ShadowMapPadding;
};

float2 NDCToUV(float3 NDC)
{
    float2 UV = (NDC.xy * 0.5) + 0.5;
    UV.y = 1 - UV.y;
    return UV;
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

float PointShadowCalculation(FPointLightInfo LightInfo, float3 WorldPos,int index)
{
    // 1) 광원→조각 방향 (큐브맵 샘플링 좌표)
    float3 Dir = normalize(WorldPos - LightInfo.Position);

    // 2) 해당 face의 뷰·프로젝션 적용
    int face = GetMajorFaceIndex(Dir);
    float4 posVS = mul(float4(WorldPos,1), LightInfo.LightViewMatrix[face]);
    float4 posCS = mul(posVS,          LightInfo.LightProjectionMatrix);

    // 3) 클립스페이스 깊이
    float refDepth = posCS.z / posCS.w;

    // 4) 바이어스

    // 5) 하드웨어 비교 샘플
    float shadow = PointShadowMapArray.SampleCmpLevelZero(ShadowMapSampler, float4(Dir, index), refDepth);

    return shadow;
}





FLightingResult PointLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FPointLightInfo LightInfo = PointLights[Index];
    
    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    
    float3 ToLight = LightInfo.Position - WorldPosition;
    float Distance = length(ToLight);
    
    float Attenuation = CalculateAttenuation(Distance, LightInfo.AttenuationRadius, LightInfo.Falloff);
    if (Attenuation <= 0.0)
    {
        return Result;
    }
    
    float3 LightDir = normalize(ToLight);
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);
    
    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);

    float Shadow = PointShadowCalculation(LightInfo, WorldPosition, Index);

    Result.DiffuseFactor = DiffuseFactor * LightInfo.LightColor.rgb * Attenuation * LightInfo.Intensity * Shadow;
    Result.SpecularFactor = SpecularFactor * LightInfo.LightColor.rgb * Attenuation * LightInfo.Intensity * Shadow;
    return Result;
}

FLightingResult SpotLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FSpotLightInfo LightInfo = SpotLights[Index];

    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    
    float3 ToLight = LightInfo.Position - WorldPosition;
    float Distance = length(ToLight);
    
    float Attenuation = CalculateAttenuation(Distance, LightInfo.AttenuationRadius, LightInfo.Falloff);

    float3 LightDir = ToLight / Distance;
    float ConeAttenuation = CalculateConeAttenuation(LightDir, normalize(LightInfo.Direction), LightInfo.AttenuationRadius, LightInfo.Falloff, LightInfo.InnerRad, LightInfo.OuterRad);
    
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);

    row_major matrix vp = mul(LightInfo.LightViewMatrix, LightInfo.LightProjectionMatrix);
    float4 LightPos = mul(float4(WorldPosition, 1.f), vp);
    float3 ShadowMapNDC = LightPos.xyz / LightPos.w;
    float2 LightUV = NDCToUV(ShadowMapNDC);
    float Depth = LightPos.z / LightPos.w;
    float Shadow = 1.0f;
    if (all(LightUV >= 0.0f && LightUV <= 1.0f))
    {
        Shadow = SpotShadowMap.SampleCmpLevelZero(ShadowMapSampler, LightUV, Depth);
    }

    Result.DiffuseFactor = DiffuseFactor * LightInfo.LightColor.rgb * LightInfo.Intensity * Attenuation * ConeAttenuation * Shadow;
    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);
    
#ifdef LIGHTING_MODEL_LAMBERT
    return Result;
#endif

    Result.SpecularFactor = SpecularFactor * LightInfo.LightColor.rgb * LightInfo.Intensity * Attenuation * ConeAttenuation * Shadow;
    return Result;
}





bool InRange(float val, float min, float max)
{
    return (min <= val && val <= max);
}

float ShadowCalculation(int nIndex, float3 WorldPos)
{
    FDirectionalLightInfo LightInfo = Directional[nIndex];

    float4 LightView = mul(float4(WorldPos,1.0), LightInfo.LightViewMatrix);
    float4 LightClipPos = mul(LightView, LightInfo.LightProjectionMatrix);
    float3 ShadowMapNDC = LightClipPos.xyz / LightClipPos.w;
    float2 ShadowMapUV = NDCToUV(ShadowMapNDC);
    float LightDistance = ShadowMapNDC.z;  
  
    // float Bias = 0.0005;
    // LightDistance -= Bias;

    float Shadow = 0.f;
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
                Shadow += DirectionalShadowMap.SampleCmpLevelZero(ShadowMapSampler, SampleUV, LightDistance).r;
            }else
            {
                Shadow += 1.f;
            }
        }
    }
    Shadow /= 9;

    return Shadow;
}

FLightingResult DirectionalLight(int nIndex, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FDirectionalLightInfo LightInfo = Directional[nIndex];

    FLightingResult Result;
    Result.DiffuseFactor = float3(0, 0, 0);
    Result.SpecularFactor = float3(0, 0, 0);
    
    float3 LightDir = normalize(-LightInfo.Direction);
    float3 ViewDir = normalize(WorldViewPosition - WorldPosition);
    float DiffuseFactor = CalculateDiffuse(WorldNormal, LightDir);
    
    float Shadow = ShadowCalculation(nIndex, WorldPosition);

    Result.DiffuseFactor = DiffuseFactor * LightInfo.Intensity * LightInfo.LightColor.rgb * Shadow;
#ifdef LIGHTING_MODEL_LAMBERT
    return Result;
#else
    
    float SpecularFactor = CalculateSpecular(WorldNormal, LightDir, ViewDir, Material.SpecularScalar);
    Result.SpecularFactor = SpecularFactor * LightInfo.Intensity * LightInfo.LightColor.rgb * Shadow;
#endif
    return Result;
}


FLightingResult Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition)
{
    FLightingResult Result = (FLightingResult)0;
    
    FLightingResult tmp;
    
    // 다소 비효율적일 수도 있음.
    [unroll(MAX_POINT_LIGHT)]
    for (int i = 0; i < PointLightsCount; i++)
    {
        tmp = PointLight(i, WorldPosition, WorldNormal, WorldViewPosition);
        Result.DiffuseFactor += tmp.DiffuseFactor;
        Result.SpecularFactor += tmp.SpecularFactor;
        
    }    
    [unroll(MAX_SPOT_LIGHT)]
    for (int j = 0; j < SpotLightsCount; j++)
    {
        tmp = SpotLight(j, WorldPosition, WorldNormal, WorldViewPosition);
        Result.DiffuseFactor += tmp.DiffuseFactor;
        Result.SpecularFactor += tmp.SpecularFactor;
    }
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < DirectionalLightsCount; k++)
    {
        tmp = DirectionalLight(k, WorldPosition, WorldNormal, WorldViewPosition);
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
