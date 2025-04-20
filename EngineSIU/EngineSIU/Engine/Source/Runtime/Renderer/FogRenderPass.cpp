#include "FogRenderPass.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Define.h"
#include "Engine/Classes/GameFramework/Actor.h"
#include <wchar.h>
#include <UObject/UObjectIterator.h>
#include <Engine/Engine.h>

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "PropertyEditor/ShowFlags.h"

FFogRenderPass::FFogRenderPass()
{
}

FFogRenderPass::~FFogRenderPass()
{
    ReleaseShader();
}

void FFogRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateShader();
    CreateBlendState();
    CreateSampler();
}

void FFogRenderPass::CreateShader()
{
    // 정점 셰이더 및 입력 레이아웃 생성
    HRESULT hr = ShaderManager->AddVertexShader(L"FogVertexShader", L"Shaders/FogShader.hlsl", "mainVS");
    if (FAILED(hr))
    {
        return;
    }
    // 픽셀 셰이더 생성
    hr = ShaderManager->AddPixelShader(L"FogPixelShader", L"Shaders/FogShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    
    // 생성된 셰이더와 입력 레이아웃 획득
    VertexShader = ShaderManager->GetVertexShaderByKey(L"FogVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"FogPixelShader");
}

void FFogRenderPass::ReleaseShader()
{
}

void FFogRenderPass::OnShaderReload()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"FogVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"FogPixelShader");
}

void FFogRenderPass::PrepareRender()
{
    for (const auto iter : TObjectRange<UHeightFogComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            FogComponents.Add(iter);
        }
    }
}

void FFogRenderPass::ClearRenderArr()
{
    FogComponents.Empty();
}

void FFogRenderPass::PrepareRenderState()
{
    // 셰이더 설정
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);

    TArray<FString> PSBufferKeys = {
        TEXT("FFogConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);
}

void FFogRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    
    if (ViewMode == EViewModeIndex::VMI_Wireframe || FogComponents.Num() <= 0 || !(ShowFlag & static_cast<uint64>(EEngineShowFlags::SF_Fog)))
    {
        return;
    }

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const EResourceType ResourceType = EResourceType::ERT_PP_Fog; 
    FRenderTargetRHI* RenderTargetRHI = Viewport->GetViewportResource()->GetRenderTarget(ResourceType);

    ViewportResource->ClearRenderTarget(Graphics->DeviceContext, ResourceType);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);
    Graphics->DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);

    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, &ViewportResource->GetDepthStencilSRV());
    
    OnShaderReload();
    PrepareRenderState();
    
    for (const auto& Fog : FogComponents)
    {
        if (Fog->GetFogDensity() > 0)
        {
            UpdateFogConstant(Fog);

            Graphics->DeviceContext->Draw(6, 0);
        }
    }

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);
}

void FFogRenderPass::UpdateFogConstant(UHeightFogComponent* Fog)
{
    FFogConstants Constants; 
    {
        Constants.FogColor = Fog->GetFogColor();
        Constants.FogDensity = Fog->GetFogDensity();
        Constants.FogDistanceWeight = Fog->GetFogDistanceWeight();
        Constants.FogHeightFalloff = Fog->GetFogHeightFalloff();
        Constants.StartDistance = Fog->GetStartDistance();
        Constants.EndDistance = Fog->GetEndDistance();
        Constants.FogHeight = Fog->GetWorldLocation().Z;
    }
    //상수버퍼 업데이트
    BufferManager->UpdateConstantBuffer(TEXT("FFogConstants"), Constants);
}

void FFogRenderPass::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Graphics->Device->CreateBlendState(&blendDesc, &BlendState);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"AlphaBlendState 생성에 실패했습니다!", L"Error", MB_ICONERROR | MB_OK);
    }
}

void FFogRenderPass::CreateSampler()
{
    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    Graphics->Device->CreateSamplerState(&SamplerDesc, &Sampler);
}
