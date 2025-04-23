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

    // viewport for shadow map
    ShadowViewport.Width = FViewportResource::ShadowMapWidth; //1024;
    ShadowViewport.Height = FViewportResource::ShadowMapHeight; //1024
    ShadowViewport.MinDepth = 0.0f;
    ShadowViewport.MaxDepth = 1.0f;
    ShadowViewport.TopLeftX = 0;
    ShadowViewport.TopLeftY = 0;

    PointShadowViewport.Width = 1024; //1024;
    PointShadowViewport.Height = 1024; //1024
    PointShadowViewport.MinDepth = 0.0f;
    PointShadowViewport.MaxDepth = 1.0f;
    PointShadowViewport.TopLeftX = 0;
    PointShadowViewport.TopLeftY = 0;

    CreateShader();
}

void FUpdateLightBufferPass::PrepareRenderState()
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerShadowMapBack);
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

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
    BakeShadowMap(Viewport);

    UpdateLightBuffer();
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLights.Empty();
    SpotLights.Empty();
    DirectionalLights.Empty();
    AmbientLights.Empty();
    StaticMeshComponents.Empty();
}

void FUpdateLightBufferPass::OnShaderReload()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"ShadowMapVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
}


void FUpdateLightBufferPass::BakeShadowMap(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();

    int DirectionalLightsCount = 0;
    int SpotLightCount = 0;
    float TextureSize = 4096.0f;

    OnShaderReload();
    PrepareRenderState();

    UINT OriginalViewportCount = 1;
    D3D11_VIEWPORT OriginalViewport = {};
    Graphics->DeviceContext->RSGetViewports(&OriginalViewportCount, &OriginalViewport);
    Graphics->DeviceContext->OMSetRenderTargets(1, &ViewportResource->GetSpotShadowMapRTV(), ViewportResource->GetSpotShadowMapDSV());
    
    int TileSize = 1024;
    int NumTilesPerRow = 4;

    for (auto Light : SpotLights)
    {
        int x = SpotLightCount % NumTilesPerRow;
        int y = SpotLightCount / NumTilesPerRow;

        ShadowViewport.TopLeftX = x * TileSize;
        ShadowViewport.TopLeftY = y * TileSize;
        ShadowViewport.Width = TileSize;
        ShadowViewport.Height = TileSize;

        Graphics->DeviceContext->RSSetViewports(1, &ShadowViewport);
        if (SpotLightCount < MAX_SPOT_LIGHT)
        {
            //Light 기준 Camera Update
            FVector LightPos = Light->GetWorldLocation();
            FVector LightDir = Light->GetDirection();
            FVector TargetPos = LightPos + LightDir;

            FCameraConstantBuffer LightViewCameraConstant;
            LightViewCameraConstant.ViewMatrix = JungleMath::CreateViewMatrix(LightPos, TargetPos, FVector(0, 0, 1));
            
            Light->ViewMatrix[0] = LightViewCameraConstant.ViewMatrix;
            Light->SetSUbUVScale(FVector2D(TileSize / TextureSize, TileSize / TextureSize));
            Light->SetSUbUVOffset(FVector2D((x * TileSize) / TextureSize, (y * TileSize) / TextureSize));

            LightViewCameraConstant.ProjectionMatrix = JungleMath::CreateProjectionMatrix(
                FMath::DegreesToRadians(Light->GetOuterDegree() * 2.0f),
                1.0f,
                0.1f,
                Light->GetAttenuationRadius()
            );

            Light->ProjectionMatrix = LightViewCameraConstant.ProjectionMatrix;

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
        }
        SpotLightCount++;
    }
    Graphics->DeviceContext->GenerateMips(ViewportResource->GetSpotShadowMapSRV());

    ShadowViewport.TopLeftX = 0;
    ShadowViewport.TopLeftY = 0;
    ShadowViewport.Width = FViewportResource::ShadowMapWidth;
    ShadowViewport.Height = FViewportResource::ShadowMapHeight;
    Graphics->DeviceContext->RSSetViewports(1, &ShadowViewport);
    Graphics->DeviceContext->OMSetRenderTargets(1, &ViewportResource->GetDirectionalShadowMapRTV(), ViewportResource->GetDirectionalShadowMapDSV());

    for (auto Light : DirectionalLights)
    {
        if (DirectionalLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            //Light기준 Camera Update
            FVector LightDir = Light->GetDirection().GetSafeNormal();
            FVector LightPos = -LightDir * (Viewport->FarClip / 2);
            FVector TargetPos = LightPos + LightDir;
            FCameraConstantBuffer LightViewCameraConstant;

            LightViewCameraConstant.ViewMatrix = JungleMath::CreateViewMatrix(LightPos, TargetPos, FVector(0, 0, 1));

            Light->ViewMatrix[0] = LightViewCameraConstant.ViewMatrix;

            LightViewCameraConstant.ProjectionMatrix = JungleMath::CreateOrthoProjectionMatrix(
                300,
                300,
                Viewport->NearClip,
                Viewport->FarClip
            );

            Light->ProjectionMatrix = LightViewCameraConstant.ProjectionMatrix;

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

    Graphics->DeviceContext->GenerateMips(ViewportResource->GetDirectionalShadowMapSRV());
    
    // 0:+X, 1:-X, 2:+Y, 3:-Y, 4:+Z, 5:-Z 순서로 Face 지정
    static const FVector LookDirections[6] = {
        FVector( +1,  0,  0 ),  // +X
        FVector( -1,  0,  0 ),  // -X
        FVector(  0, +1,  0 ),  // +Y
        FVector(  0, -1,  0 ),  // -Y
        FVector(  0,  0, +1 ),  // +Z
        FVector(  0,  0, -1 )   // -Z
    };

    // Z축(up)과 colinear하지 않도록, Z‑face만 Y축을 up으로 사용
    static const FVector UpDirections[6] = {
        FVector( 0, 1, 0 ),  // +X face → up = +Z
        FVector( 0, 1, 0 ),  // -X face → up = +Z
        FVector( 0, 0, -1 ),  // +Y face → up = +Z
        FVector( 0, 0, 1 ),  // -Y face → up = +Z
        FVector( 0, 1, 0 ),  // +Z face → up = +Y
        FVector( 0, 1, 0 )   // -Z face → up = +Y
    };

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(4, 1, nullSRV);

    int lightindex=0;
    for (auto Light : PointLights)
    {
        FVector LightPos = Light->GetWorldLocation();
        float Near = 0.01f;
        float Far = Light->GetAttenuationRadius();

        for (int Face = 0; Face < 6; ++Face)
        {
            ID3D11RenderTargetView* FaceRTV = ViewportResource->GetPointShadowMapRTV(lightindex * 6 + Face);
            ID3D11DepthStencilView* FaceDSV = ViewportResource->GetPointShadowMapDSV(lightindex * 6 + Face);
            float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            
            Graphics->DeviceContext->RSSetViewports(1, &PointShadowViewport);
            Graphics->DeviceContext->ClearRenderTargetView(FaceRTV, ClearColor);
            Graphics->DeviceContext->ClearDepthStencilView(FaceDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
            Graphics->DeviceContext->OMSetRenderTargets(1, &FaceRTV, FaceDSV);
            
            FCameraConstantBuffer LightViewCamera;
            LightViewCamera.ViewMatrix = JungleMath::CreateViewMatrix(
                LightPos,
                LightPos + LookDirections[Face],
                UpDirections[Face]
            );
            LightViewCamera.ProjectionMatrix = JungleMath::CreateProjectionMatrix(
                FMath::DegreesToRadians(90.0f), 1.0f, Near, Far
            );

            Light->ViewMatrix[Face] = LightViewCamera.ViewMatrix;
            Light->ProjectionMatrix = LightViewCamera.ProjectionMatrix;
            Light->index = lightindex;

            BufferManager->UpdateConstantBuffer(TEXT("FCameraConstantLightViewBuffer"), LightViewCamera);

            for (UStaticMeshComponent* Comp : StaticMeshComponents)
            {
                if (!Comp || !Comp->GetStaticMesh()) continue;
                OBJ::FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
                if (!RenderData) continue;

                UpdateObjectConstant(Comp->GetWorldMatrix());
                RenderPrimitive(RenderData);
            }
        }

        lightindex++;
    }

    Graphics->DeviceContext->GenerateMips(ViewportResource->GetPointShadowMapArraySRV());
    Graphics->DeviceContext->RSSetViewports(OriginalViewportCount, &OriginalViewport);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FUpdateLightBufferPass::UpdateLightBuffer() const
{
    FLightInfoBuffer LightBufferData = {};

    int DirectionalLightsCount = 0;
    int PointLightsCount = 0;
    int SpotLightsCount = 0;
    int AmbientLightsCount = 0;

    for (auto Light : SpotLights)
    {
        if (SpotLightsCount < MAX_SPOT_LIGHT)
        {
            LightBufferData.SpotLights[SpotLightsCount] = Light->GetSpotLightInfo();
            LightBufferData.SpotLights[SpotLightsCount].Position = Light->GetWorldLocation();
            LightBufferData.SpotLights[SpotLightsCount].Direction = Light->GetDirection();
            LightBufferData.SpotLights[SpotLightsCount].LightViewMatrix = Light->ViewMatrix[0];
            LightBufferData.SpotLights[SpotLightsCount].LightProjectionMatrix = Light->ProjectionMatrix;
            LightBufferData.SpotLights[SpotLightsCount].SubUVScale = Light->GetSubUVScale();
            LightBufferData.SpotLights[SpotLightsCount].SubUVOffset = Light->GetSubUVOffset();
            LightBufferData.SpotLights[SpotLightsCount].bCastShadow = Light->bCastShadow ? 1 : 0;
            LightBufferData.SpotLights[SpotLightsCount].Sharpness = Light->ShadowSharpen;
            SpotLightsCount++;
        }
    }

    for (auto Light : PointLights)
    {
        if (PointLightsCount < MAX_POINT_LIGHT)
        {
            LightBufferData.PointLights[PointLightsCount] = Light->GetPointLightInfo();
            LightBufferData.PointLights[PointLightsCount].Position = Light->GetWorldLocation();
            LightBufferData.PointLights[PointLightsCount].bCastShadow = Light->bCastShadow ? 1 : 0;
            LightBufferData.PointLights[PointLightsCount].Sharpness = Light->ShadowSharpen;
            for(int i = 0; i<6; i++)
                LightBufferData.PointLights[PointLightsCount].LightViewMatrix[i] = Light->ViewMatrix[i];
            LightBufferData.PointLights[PointLightsCount].LightProjectionMatrix = Light->ProjectionMatrix;
            PointLightsCount++;
        }
    }

    for (auto Light : DirectionalLights)
    {
        if (DirectionalLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            LightBufferData.Directional[DirectionalLightsCount] = Light->GetDirectionalLightInfo();
            LightBufferData.Directional[DirectionalLightsCount].Direction = Light->GetDirection();
            LightBufferData.Directional[DirectionalLightsCount].LightViewMatrix = Light->ViewMatrix[0];
            LightBufferData.Directional[DirectionalLightsCount].LightProjectionMatrix = Light->ProjectionMatrix;
            LightBufferData.Directional[DirectionalLightsCount].bCastShadow = Light->bCastShadow ? 1 : 0;
            LightBufferData.Directional[DirectionalLightsCount].Sharpness = Light->ShadowSharpen;
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
    LightBufferData.ShadowMapWidth = FViewportResource::ShadowMapWidth;
    LightBufferData.ShadowMapHeight = FViewportResource::ShadowMapHeight;
    LightBufferData.FilterMode = FEngineLoop::ResourceManager.ShaderFilterMode;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
}


void FUpdateLightBufferPass::CreateShader()
{
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"ShadowMapVertexShader", L"Shaders/ShadowMapVertexShader.hlsl", "mainVS",
                                                              ShaderManager->StaticMeshLayoutDesc, ARRAYSIZE(ShaderManager->StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }

    hr = ShaderManager->AddPixelShader(L"ShadowMapPixelShader", L"Shaders/ShadowMapPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }

    VertexShader = ShaderManager->GetVertexShaderByKey(L"ShadowMapVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"ShadowMapPixelShader");
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
