#pragma once
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"
#define MAX_AMBIENT_LIGHT 16
#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16

struct FAmbientLightInfo
{
    FLinearColor AmbientColor;         // RGB + alpha
};

struct FDirectionalLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Direction;   // 정규화된 광선 방향 (월드 공간 기준)
    float   Intensity;   // 밝기

    FMatrix LightViewMatrix;
    FMatrix LightProjectionMatrix;

    int bCastShadow;
    int Sharpness;
    FVector2D Pad;
};

struct FPointLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Position;    // 월드 공간 위치
    float   AttenuationRadius;      // 감쇠가 0이 되는 거리

    int     Type;        // 라이트 타입 구분용 (예: 1 = Point)
    float   Intensity;   // 밝기
    float   Falloff;
    int  bCastShadow;

    FMatrix LightViewMatrix[6];
    FMatrix LightProjectionMatrix;

    int Sharpness;
    FVector Pad;
};

struct FSpotLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Position;       // 월드 공간 위치
    float   AttenuationRadius;         // 감쇠 거리

    FVector Direction;      // 빛이 향하는 방향 (normalize)
    float   Intensity;      // 밝기

    int     Type;           // 라이트 타입 구분용 (예: 2 = Spot)
    float   InnerRad; // cos(inner angle)
    float   OuterRad; // cos(outer angle)
    float   Falloff;

    int bCastShadow;
    int Sharpness;
    FVector2D Pad;

    FMatrix LightViewMatrix;
    FMatrix LightProjectionMatrix;

    FVector2D SubUVScale;
    FVector2D SubUVOffset;
};

struct alignas(16) FLightInfoBuffer
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
