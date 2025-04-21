#include "BillboardRenderPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "UnrealEd/EditorViewportClient.h"

#include "Components/BillboardComponent.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/TextComponent.h"
#include "Engine/EditorEngine.h"

#include "EngineLoop.h"
#include "UnrealClient.h"

#include "World/World.h"

FBillboardRenderPass::FBillboardRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , ResourceType(EResourceType::ERT_Scene)
    , VertexShader(nullptr)
    , PixelShader(nullptr)
    , InputLayout(nullptr)
{
}

FBillboardRenderPass::~FBillboardRenderPass()
{
}

void FBillboardRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    CreateShader();
}

void FBillboardRenderPass::PrepareRender()
{
    BillboardComps.Empty();
    for (const auto iter : TObjectRange<UBillboardComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            BillboardComps.Add(iter);
        }
    }
}

void FBillboardRenderPass::PrepareTextureShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 0, EShaderStage::Vertex);
}

void FBillboardRenderPass::PrepareSubUVConstant() const
{
    BufferManager->BindConstantBuffer(TEXT("FSubUVConstant"), 1, EShaderStage::Pixel);
}

void FBillboardRenderPass::UpdateSubUVConstant(FVector2D uvOffset, FVector2D uvScale) const
{
    FSubUVConstant data;
    data.uvOffset = uvOffset;
    data.uvScale = uvScale;

    BufferManager->UpdateConstantBuffer(TEXT("FSubUVConstant"), data);
}

void FBillboardRenderPass::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FBillboardRenderPass::RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, numVertices);

    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);
    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FBillboardRenderPass::RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, numVertices);

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FBillboardRenderPass::CreateShader()
{
    // Billboard 셰이더 생성
    D3D11_INPUT_ELEMENT_DESC TextureLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"VertexBillboardShader", L"Shaders/VertexBillboardShader.hlsl", "main", TextureLayoutDesc, ARRAYSIZE(TextureLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    
    hr = ShaderManager->AddPixelShader(L"PixelBillboardShader", L"Shaders/PixelBillboardShader.hlsl", "main");
    if (FAILED(hr))
    {
        return;
    }
    
    VertexShader = ShaderManager->GetVertexShaderByKey(L"VertexBillboardShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"VertexBillboardShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"PixelBillboardShader");
}

void FBillboardRenderPass::ReleaseShader()
{
}

void FBillboardRenderPass::OnShaderReload()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"VertexBillboardShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"VertexBillboardShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"PixelBillboardShader");
}

void FBillboardRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();

    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, ViewportResource->GetDepthStencilView());

    OnShaderReload();

    PrepareTextureShader();
    PrepareSubUVConstant();

    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    // 각 Billboard에 대해 렌더링 처리
    for (auto BillboardComp : BillboardComps)
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        
        FMatrix Model = BillboardComp->CreateBillboardMatrix();
        FVector4 UUIDColor = BillboardComp->EncodeUUID() / 255.0f;
        bool bIsSelected = (Engine && Engine->GetSelectedActor() == BillboardComp->GetOwner());
        UpdateObjectConstant(Model, UUIDColor, bIsSelected);

        if (UParticleSubUVComponent* SubUVParticle = Cast<UParticleSubUVComponent>(BillboardComp))
        {
            UpdateSubUVConstant(SubUVParticle->GetUVOffset(), SubUVParticle->GetUVScale());

            RenderTexturePrimitive(
                VertexInfo.VertexBuffer,
                VertexInfo.NumVertices,
                IndexInfo.IndexBuffer,
                IndexInfo.NumIndices,
                SubUVParticle->Texture->TextureSRV,
                SubUVParticle->Texture->SamplerState
            );
        }
        else if (UTextComponent* TextComp = Cast<UTextComponent>(BillboardComp))
        {
            FBufferInfo Buffers;
            float Height = TextComp->Texture->Height;
            float Width = TextComp->Texture->Width;
            BufferManager->CreateUnicodeTextBuffer(TextComp->GetText(), Buffers, Width, Height, TextComp->GetColumnCount(), TextComp->GetRowCount());

            UpdateSubUVConstant(FVector2D(), FVector2D(1, 1));

            RenderTextPrimitive(
                Buffers.VertexInfo.VertexBuffer,
                Buffers.VertexInfo.NumVertices,
                TextComp->Texture->TextureSRV,
                TextComp->Texture->SamplerState
            );
        }
        else
        {
            UpdateSubUVConstant(FVector2D(BillboardComp->finalIndexU, BillboardComp->finalIndexV), FVector2D(1, 1));

            RenderTexturePrimitive(
                VertexInfo.VertexBuffer,
                VertexInfo.NumVertices,
                IndexInfo.IndexBuffer,
                IndexInfo.NumIndices,
                BillboardComp->Texture->TextureSRV,
                BillboardComp->Texture->SamplerState
            );
        }
    }

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FBillboardRenderPass::SetupVertexBuffer(ID3D11Buffer* pVertexBuffer, UINT numVertices) const
{
    UINT Stride = sizeof(FVertexTexture);
    UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FBillboardRenderPass::ClearRenderArr()
{
    BillboardComps.Empty();
}
