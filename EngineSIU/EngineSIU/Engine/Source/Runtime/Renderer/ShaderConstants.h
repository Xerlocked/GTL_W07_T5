// Shader에 들어가는 (특히 static mesh) 상수 버퍼 구조체의 정의입니다.
// 추가하실 때 ShaderConstants.hlsli도 동일하게 맞춰주세요.
// 슬롯 13번은 디버그 전용입니다.
// struct에 alignas가 붙어있으니 꼭 struct의 메모리 구조를 보면서 확인해주세요
// hlsl에서는 float3 float3을 연속으로 struct에 넣으면 자동으로 padding 4byte가 붙습니다.
// 이상한 값이 들어온다면 hlsl파일에 패딩을 넣어주세요.

#pragma once
#include <d3d11.h>
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"
#include "Math/Color.h"

// 아래의 두개 다 수정하기
#define MACRO_FCONSTANT_NUM_MAX_DIRLIGHT 10
#define MACRO_FCONSTANT_NUM_MAX_POINTLIGHT 10
#define MACRO_FCONSTANT_NUM_MAX_SPOTLIGHT 10

#define FCONSTANT_STRINGIFY(x) #x
#define FCONSTANT_TOSTRING(x) FCONSTANT_STRINGIFY(x)

// hlsl파일에 들어갈 macro define
const D3D_SHADER_MACRO defines[] =
{
    "FCONSTANT_NUM_DIRLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_DIRLIGHT),
    "FCONSTANT_NUM_POINTLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_POINTLIGHT),
    "FCONSTANT_NUM_SPOTLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_SPOTLIGHT),
    NULL, NULL
};


struct FConstantBuffersStaticMesh
{
    ID3D11Buffer* Camera00;
    ID3D11Buffer* Light01;
    ID3D11Buffer* Actor03;
    ID3D11Buffer* Texture05;
    ID3D11Buffer* Mesh06;
};

/// <summary>
/// Per-Scene 상수버퍼 : b0
/// </summary>
struct alignas(16) FConstantBufferCamera
{
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
    alignas(16) FVector CameraPos;
    alignas(16) FVector CameraLookAt;
};

/////////////////////////////////////////////////////////////////////////
// 디버그용
/////////////////////////////////////////////////////////////////////////

/// <summary>
/// Debug용 AABB 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugAABB
{
    FVector Position;
    float Padding1;
    
    FVector Extent;
    float Padding2;
};

/// <summary>
/// Debug용 sphere 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugSphere
{
    FVector Position;
    float Radius;
};

/// <summary>
/// Debug용 cone 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugCone
{
    FVector ApexPosition;
    float Radius;
    FVector Direction;
    float Angle;
    FLinearColor Color;
};

/// <summary>
/// Debug용 grid 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugGrid
{
    FMatrix InverseViewProj;
};

/// <summary>
/// Debug용 grid 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugIcon
{
    FVector Position;
    float Scale;
};

/// <summary>
/// Debug용 arrow 상수버퍼 : b13
/// </summary>
struct FConstantBufferDebugArrow
{
    FVector Position;
    float ArrowScaleXYZ;
    
    FVector Direction;
    float ArrowScaleZ;
};

