#include "StaticMeshRenderPass.h"

#include <array>

#include "EngineLoop.h"
#include "World/World.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "Math/JungleMath.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "Components/StaticMeshComponent.h"

#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"

#include "PropertyEditor/ShowFlags.h"

#include "UnrealEd/EditorViewportClient.h"


FStaticMeshRenderPass::FStaticMeshRenderPass()
    : VertexShader(nullptr)
    , PixelShader(nullptr)
    , InputLayout(nullptr)
    , BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FStaticMeshRenderPass::~FStaticMeshRenderPass()
{
    ReleaseShader();
}

void FStaticMeshRenderPass::CreateShader()
{
    // Begin Debug Shaders
    HRESULT hr = ShaderManager->AddPixelShader(L"StaticMeshPixelShaderDepth", L"Shaders/StaticMeshPixelShaderDepth.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"StaticMeshPixelShaderWorldNormal", L"Shaders/StaticMeshPixelShaderWorldNormal.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    // End Debug Shaders

#pragma region UberShader
    D3D_SHADER_MACRO DefinesGouraud[] =
    {
        { GOURAUD, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"GOURAUD_StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefinesGouraud);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D_SHADER_MACRO DefinesLambert[] =
    {
        { LAMBERT, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"LAMBERT_StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefinesLambert);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D_SHADER_MACRO DefinesBlinnPhong[] =
    {
        { PHONG, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"PHONG_StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefinesBlinnPhong);
    if (FAILED(hr))
    {
        return;
    }
    
#pragma endregion UberShader
    
    VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    
    PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");
    DebugDepthShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderDepth");
    DebugWorldNormalShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldNormal");
}

void FStaticMeshRenderPass::ReleaseShader()
{
    
}

void FStaticMeshRenderPass::ChangeViewMode(EViewModeIndex ViewModeIndex)
{
    switch (ViewModeIndex)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"GOURAUD_StaticMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"GOURAUD_StaticMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"GOURAUD_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_BlinnPhong:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Wireframe:
    case EViewModeIndex::VMI_Unlit:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");
        UpdateLitUnlitConstant(0);
        break;
    }
}

void FStaticMeshRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateShader();

    D3D11_SAMPLER_DESC ShadowMapSamplerDesc;
    ZeroMemory(&ShadowMapSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
    ShadowMapSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowMapSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowMapSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowMapSamplerDesc.BorderColor[0] = 0.0f;
    ShadowMapSamplerDesc.MinLOD = 0.f;
    ShadowMapSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // 밉맵/블러 효과를 내고 싶다면 MaxLOD를 D3D11_FLOAT32_MAX로!
    ShadowMapSamplerDesc.MipLODBias = 0.f;
    ShadowMapSamplerDesc.MaxAnisotropy = 1; //0은 무효, 1 이상(보통 1~16)로 설정 권장 (Linear 필터에서는 큰 영향 없음)

    ShadowMapSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    ShadowMapSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    
    Graphics->Device->CreateSamplerState(&ShadowMapSamplerDesc, &ShadowMapSampler);

    // Comparison
    ShadowMapSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    ShadowMapSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    
    Graphics->Device->CreateSamplerState(&ShadowMapSamplerDesc, &ShadowMapCompareSampler);
}

void FStaticMeshRenderPass::PrepareRender()
{
    for (const auto iter : TObjectRange<UStaticMeshComponent>())
    {
        if (!Cast<UGizmoBaseComponent>(iter) && iter->GetWorld() == GEngine->ActiveWorld)
        {
            StaticMeshComponents.Add(iter);
        }
    }
}

void FStaticMeshRenderPass::PrepareRenderState(const std::shared_ptr<FEditorViewportClient>& Viewport) 
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    ChangeViewMode(ViewMode);
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants"),
        TEXT("FSubMeshConstants"),
        TEXT("FTextureConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);

    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 1, EShaderStage::Vertex);

    //ChangeViewMode(ViewMode);

    // Rasterizer
    if (ViewMode == EViewModeIndex::VMI_Wireframe)
    {
        Graphics->DeviceContext->RSSetState(Graphics->RasterizerWireframeBack);
    }
    else
    {
        Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    }

    // Pixel Shader
    if (ViewMode == EViewModeIndex::VMI_SceneDepth)
    {
        Graphics->DeviceContext->PSSetShader(DebugDepthShader, nullptr, 0);
    }
    else if (ViewMode == EViewModeIndex::VMI_WorldNormal)
    {
        Graphics->DeviceContext->PSSetShader(DebugWorldNormalShader, nullptr, 0);
    }
    else
    {
        Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    }
}

void FStaticMeshRenderPass::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FStaticMeshRenderPass::UpdateLitUnlitConstant(int32 isLit) const
{
    FLitUnlitConstants Data;
    Data.bIsLit = isLit;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);
}

void FStaticMeshRenderPass::RenderPrimitive(OBJ::FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const
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
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (SubMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (OverrideMaterials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
        }
        else
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
        }

        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FStaticMeshRenderPass::PrepareShadowMap(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();

    ID3D11ShaderResourceView* DirectionalLightShadowMapSRV = ViewportResource->GetDirectionalShadowMapSRV();
    
    ID3D11ShaderResourceView* SpotLightShadowMapSRV = ViewportResource->GetSpotShadowMapSRV();

    ID3D11ShaderResourceView* PointLightShadowMapSRV = ViewportResource->GetPointShadowMapArraySRV();

    ID3D11ShaderResourceView* DirectionalLightShadowMapCompareSRV = ViewportResource->GetDirectionalShadowMapCompareSRV();
    
    ID3D11ShaderResourceView* SpotLightShadowMapCompareSRV = ViewportResource->GetSpotShadowMapCompareSRV();

    Graphics->DeviceContext->PSSetSamplers(2, 1, &ShadowMapSampler);

    Graphics->DeviceContext->PSSetSamplers(3, 1, &ShadowMapCompareSampler);
    
    Graphics->DeviceContext->PSSetShaderResources(2, 1, &DirectionalLightShadowMapSRV);
    
    Graphics->DeviceContext->PSSetShaderResources(3, 1, &SpotLightShadowMapSRV);

    Graphics->DeviceContext->PSSetShaderResources(4, 1, &PointLightShadowMapSRV);
    
    Graphics->DeviceContext->PSSetShaderResources(5, 1, &DirectionalLightShadowMapCompareSRV);

    Graphics->DeviceContext->PSSetShaderResources(6, 1, &SpotLightShadowMapCompareSRV);
}

void FStaticMeshRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, ViewportResource->GetDepthStencilView());
    ViewportResource->ClearRenderTarget(Graphics->DeviceContext, ResourceType);
    Graphics->DeviceContext->ClearDepthStencilView(ViewportResource->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    OnShaderReload();
    PrepareRenderState(Viewport);

    PrepareShadowMap(Viewport);

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
        
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        
        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());
        
        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);
        
        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), WorldMatrix);
        }
    }

    // 렌더 타겟 해제
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FStaticMeshRenderPass::ClearRenderArr()
{
    StaticMeshComponents.Empty();
}

void FStaticMeshRenderPass::OnShaderReload()
{
    // 공통 셰이더
    DebugDepthShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderDepth");
    DebugWorldNormalShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldNormal");

    // ViewModeIndex에 따라 선택적으로 다시 설정
    // 현재 ViewModeIndex를 저장하고 있다면 그것 기준으로 ChangeViewMode 호출 가능
    ChangeViewMode(EViewModeIndex::VMI_Lit_BlinnPhong); // 또는 현재 모드 기억해뒀다가 호출
}
