
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

SamplerState ShadowMapSampler : register(s2);

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

    int Sharpness;
    float3 Pad;
    
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

    int bCastShadow;
    int Sharpness;
    float2 Pad;

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

float CalculateShadowByPCF(Texture2D ShadowMap, float2 ShadowMapUV, int Sharpness, int LightDepth)
{
    // 1) 초기화
    float Shadow = 0.0f;

    // 2) 오프셋: 텍셀 크기
    float2 TexelSize = float2(1.0f / ShadowMapWidth, 1.0f / ShadowMapHeight);

    // 3×3 커널 순회
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 SampleUV = ShadowMapUV + TexelSize * float2(i, j);

            // UV 범위 체크
            if (all(SampleUV >= 0.0f) && all(SampleUV <= 1.0f))
            {
                // VSM 텍스처의 R 채널(평균 깊이)만 비교
                float SampleMean = ShadowMap.SampleLevel(ShadowMapSampler, SampleUV, Sharpness).r;
                Shadow += (LightDepth <= SampleMean) ? 1.0f : 0.0f;
            }
            else
            {
                // 범위 밖은 빛을 받는 것으로 처리
                Shadow += 1.0f;
            }
        }
    }

    // 9개 샘플 평균
    return Shadow / 9.0f;
}


float CalculateShadowByVSM(Texture2D ShadowMap, float2 ShadowMapUV, float LightDistance, int Sharpness)
{
    // 0에 가까울수록 그림자 1이면 빛 그대로받는거
    float Shadow = 1.f;


    ///////////////////////////////////////////////////////////////
    /// VSM
    ///  // One-tailed inequality valid if t > Moments.x
    float2 moments = ShadowMap.SampleLevel(ShadowMapSampler, ShadowMapUV, Sharpness).rg;
    float mean = moments.x; //mean depth 평균
    float mean2 = moments.y; //mean2 detph^2 평균
    
    float p = (LightDistance <= mean);
    // Compute variance.
    float Variance = max(mean2 - (mean * mean), 0.00001);
    
    // Compute probabilistic upper bound.
    float d = LightDistance - mean;
    float p_max = Variance / (Variance + d * d);
    Shadow = max(p, p_max);

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

float PointShadowCalculation(FPointLightInfo LightInfo, float3 WorldPos,int index)
{

    float3 Dir = normalize(WorldPos - LightInfo.Position);

    int face = GetMajorFaceIndex(Dir);
    float4 posVS = mul(float4(WorldPos,1), LightInfo.LightViewMatrix[face]);
    float4 posCS = mul(posVS,          LightInfo.LightProjectionMatrix);

    float refDepth = posCS.z / posCS.w;

    float shadow = PointShadowMapArray.SampleLevel(ShadowMapSampler, float4(Dir, index), 0).r;


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

    float Shadow = LightInfo.bCastShadow ? PointShadowCalculation(LightInfo, WorldPosition, Index) : 1;

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

    //float SpotLightDistance = (SpotLightView.z * -1.0f) / SpotLightInfo.AttenuationRadius;
    float SpotLightDistance = SpotShadowMapNDC.z;

    float Shadow = 1.0f;

    if (SpotLightInfo.bCastShadow)
    {
        if (FilterMode == 0)
        {
            Shadow = CalculateShadowByVSM(SpotShadowMap, SpotShadowMapUV, SpotLightDistance, SpotLightInfo.Sharpness);
        }

        if (FilterMode == 1)
        {
            Shadow = CalculateShadowByPCF(SpotShadowMap, SpotShadowMapUV, SpotLightInfo.Sharpness, SpotLightDistance);
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

    float Shadow = 1.0f;

    if (DirectionalLightInfo.bCastShadow)
    {
        if (FilterMode == 0)
        {
            Shadow = CalculateShadowByVSM(DirectionalShadowMap, ShadowMapUV, LightDistance, DirectionalLightInfo.Sharpness);
        }

        if (FilterMode == 1)
        {
            Shadow = CalculateShadowByPCF(DirectionalShadowMap, ShadowMapUV, DirectionalLightInfo.Sharpness, LightDistance);
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
