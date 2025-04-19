#include "Define.h"
#include "UObject/Casts.h"
#include "UpdateLightBufferPass.h"

#include "UnrealClient.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "UObject/UObjectIterator.h"
#include "Math/JungleMath.h"

//------------------------------------------------------------------------------
// 생성자/소멸자
//------------------------------------------------------------------------------
FUpdateLightBufferPass::FUpdateLightBufferPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FUpdateLightBufferPass::~FUpdateLightBufferPass()
{
    ReleaseShader();
}

void FUpdateLightBufferPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateShader();
}

void FUpdateLightBufferPass::PrepareRenderState()
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerShadowMapFront);
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0); // 픽셀 쉐이더는 필요없음.

    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCameraConstantLightViewBuffer"), 1, EShaderStage::Vertex);
}

void FUpdateLightBufferPass::PrepareRender()
{
    for (const auto iter : TObjectRange<ULightComponentBase>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(iter))
            {
                PointLights.Add(PointLight);
            }
            else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(iter))
            {
                SpotLights.Add(SpotLight);
            }
            else if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(iter))
            {
                DirectionalLights.Add(DirectionalLight);
            }
            // Begin Test
            else if (UAmbientLightComponent* AmbientLight = Cast<UAmbientLightComponent>(iter))
            {
                AmbientLights.Add(AmbientLight);
            }
            // End Test
        }
    }

    for (const auto iter : TObjectRange<UStaticMeshComponent>())
    {
        if (!Cast<UGizmoBaseComponent>(iter) && iter->GetWorld() == GEngine->ActiveWorld)
        {
            StaticMeshComponents.Add(iter);
        }
    }
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateLightBuffer();
    
    BakeShadowMap(Viewport);
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLights.Empty();
    SpotLights.Empty();
    DirectionalLights.Empty();
    AmbientLights.Empty();
    StaticMeshComponents.Empty();
}

void FUpdateLightBufferPass::BakeShadowMap(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();

    int DirectionalLightsCount=0;
    int SpotLightCount = 0;

    PrepareRenderState();

    // Graphics->DeviceContext->ClearDepthStencilView(ViewportResource->GetSpotShadowMapDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    // Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, ViewportResource->GetSpotShadowMapDSV());
    //
    // for (auto Light : SpotLights)
    // {
    //     if (SpotLightCount < MAX_DIRECTIONAL_LIGHT)
    //     {
    //         //Light기준 Camera Update
    //         FVector LightPos = Light->GetWorldLocation();  
    //         FVector LightDir = Light->GetForwardVector().GetSafeNormal();
    //         FVector TargetPos = LightPos + LightDir;
    //         
    //         FCameraConstantBuffer LightViewCameraConstant;
    //         LightViewCameraConstant.ViewMatrix = JungleMath::CreateViewMatrix(LightPos, TargetPos, FVector(0, 0, 1));
    //
    //         float AspectRatio = Viewport->GetD3DViewport().Width / Viewport->GetD3DViewport().Height;
    //         float ViewFOV = 90;
    //         float LightNearClip = 0.1;
    //         float LightFarClip = 100.0;
    //         
    //         LightViewCameraConstant.ProjectionMatrix = JungleMath::CreateProjectionMatrix(
    //             FMath::DegreesToRadians(ViewFOV),
    //             AspectRatio,
    //             LightNearClip,
    //             LightFarClip
    //         ); //spotlight 거리만큼 Far값 + a
    //         
    //         BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantLightViewBuffer"), LightViewCameraConstant);
    //
    //         for (UStaticMeshComponent* Comp : StaticMeshComponents)
    //         {
    //             if (!Comp || !Comp->GetStaticMesh())
    //             {
    //                 continue;
    //             }
    //
    //             OBJ::FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
    //             if (RenderData == nullptr)
    //             {
    //                 continue;
    //             }
    //             
    //             FMatrix WorldMatrix = Comp->GetWorldMatrix();
    //             
    //             UpdateObjectConstant(WorldMatrix);
    //             
    //             RenderPrimitive(RenderData);
    //         }
    //
    //         SpotLightCount++;
    //     }
    // }

     Graphics->DeviceContext->ClearDepthStencilView(ViewportResource->GetDirectionalShadowMapDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
     Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, ViewportResource->GetDirectionalShadowMapDSV());

     for (auto Light : DirectionalLights)
     {
         if (DirectionalLightsCount < MAX_DIRECTIONAL_LIGHT)
         {
             //Light기준 Camera Update
             FVector LightDir = Light->GetDirection().GetSafeNormal();
             FVector LightPos = -LightDir * (Viewport->FarClip/2);
             FVector TargetPos = LightPos + LightDir;
             // FVector TargetPos = FVector::ZeroVector;
             
             FCameraConstantBuffer LightViewCameraConstant;
             LightViewCameraConstant.ViewMatrix = JungleMath::CreateViewMatrix(LightPos, TargetPos, FVector(0, 0, 1));
             
             // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
             float OrthoWidth = Viewport->OrthoSize * Viewport->AspectRatio;
             float OrthoHeight = Viewport->OrthoSize;
             LightViewCameraConstant.ProjectionMatrix = JungleMath::CreateOrthoProjectionMatrix(
                 OrthoWidth,
                 OrthoHeight,
                 Viewport->NearClip,
                 Viewport->FarClip
             );
             
             BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantLightViewBuffer"), LightViewCameraConstant);
    
             for (UStaticMeshComponent* Comp : StaticMeshComponents)
             {
                 if (!Comp || !Comp->GetStaticMesh())
                 {
                     continue;
                 }
    
                 OBJ::FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
                 if (RenderData == nullptr)
                 {
                     continue;
                 }
                 
                 FMatrix WorldMatrix = Comp->GetWorldMatrix();
                 
                 UpdateObjectConstant(WorldMatrix);
                 
                 RenderPrimitive(RenderData);
             }
    
             DirectionalLightsCount++;
         }
     }
}

void FUpdateLightBufferPass::UpdateLightBuffer() const
{
    FLightInfoBuffer LightBufferData = {};

    int DirectionalLightsCount=0;
    int PointLightsCount=0;
    int SpotLightsCount=0;
    int AmbientLightsCount=0;
    
    for (auto Light : SpotLights)
    {
        if (SpotLightsCount < MAX_SPOT_LIGHT)
        {
            LightBufferData.SpotLights[SpotLightsCount] = Light->GetSpotLightInfo();
            LightBufferData.SpotLights[SpotLightsCount].Position = Light->GetWorldLocation();
            LightBufferData.SpotLights[SpotLightsCount].Direction = Light->GetDirection();
            SpotLightsCount++;
        }
    }

    for (auto Light : PointLights)
    {
        if (PointLightsCount < MAX_POINT_LIGHT)
        {
            LightBufferData.PointLights[PointLightsCount] = Light->GetPointLightInfo();
            LightBufferData.PointLights[PointLightsCount].Position = Light->GetWorldLocation();
            PointLightsCount++;
        }
    }

    for (auto Light : DirectionalLights)
    {
        if (DirectionalLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            LightBufferData.Directional[DirectionalLightsCount] = Light->GetDirectionalLightInfo();
            LightBufferData.Directional[DirectionalLightsCount].Direction = Light->GetDirection();
            DirectionalLightsCount++;
        }
    }

    for (auto Light : AmbientLights)
    {
        if (AmbientLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            LightBufferData.Ambient[AmbientLightsCount] = Light->GetAmbientLightInfo();
            LightBufferData.Ambient[AmbientLightsCount].AmbientColor = Light->GetLightColor();
            AmbientLightsCount++;
        }
    }
    
    LightBufferData.DirectionalLightsCount = DirectionalLightsCount;
    LightBufferData.PointLightsCount = PointLightsCount;
    LightBufferData.SpotLightsCount = SpotLightsCount;
    LightBufferData.AmbientLightsCount = AmbientLightsCount;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
    
}


void FUpdateLightBufferPass::CreateShader()
{
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"ShaderMapVertexShader", L"Shaders/ShaderMapVertexShader.hlsl", "mainVS", ShaderManager->StaticMeshLayoutDesc, ARRAYSIZE(ShaderManager->StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    
    VertexShader = ShaderManager->GetVertexShaderByKey(L"ShaderMapVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
}

void FUpdateLightBufferPass::ReleaseShader()
{
    
}

void FUpdateLightBufferPass::RenderPrimitive(OBJ::FStaticMeshRenderData* RenderData) const
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &RenderData->VertexBuffer, &Stride, &Offset);

    if (RenderData->IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(RenderData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FUpdateLightBufferPass::UpdateObjectConstant(const FMatrix& WorldMatrix) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}
