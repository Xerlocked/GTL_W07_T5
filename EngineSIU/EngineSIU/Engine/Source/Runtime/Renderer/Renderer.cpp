
#include "Renderer.h"

#include <array>
#include "World/World.h"
#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "RendererHelpers.h"
#include "StaticMeshRenderPass.h"
#include "WorldBillboardRenderPass.h"
#include "EditorBillboardRenderPass.h"
#include "GizmoRenderPass.h"
#include "UpdateLightBufferPass.h"
#include "LineRenderPass.h"
#include "FogRenderPass.h"
#include "SlateRenderPass.h"
#include "EditorRenderPass.h"
#include <UObject/UObjectIterator.h>
#include <UObject/Casts.h>

#include "CompositingPass.h"
#include "PostProcessCompositingPass.h"
#include "SlateRenderPass.h"
#include "UnrealClient.h"
#include "GameFrameWork/Actor.h"

#include "PropertyEditor/ShowFlags.h"

//------------------------------------------------------------------------------
// 초기화 및 해제 관련 함수
//------------------------------------------------------------------------------
void FRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device);

    CreateConstantBuffers();
    CreateCommonShader();
    
    StaticMeshRenderPass = new FStaticMeshRenderPass();
    WorldBillboardRenderPass = new FWorldBillboardRenderPass();
    EditorBillboardRenderPass = new FEditorBillboardRenderPass();
    GizmoRenderPass = new FGizmoRenderPass();
    UpdateLightBufferPass = new FUpdateLightBufferPass();
    LineRenderPass = new FLineRenderPass();
    FogRenderPass = new FFogRenderPass();
    EditorRenderPass = new FEditorRenderPass();
    CompositingPass = new FCompositingPass();
    PostProcessCompositingPass = new FPostProcessCompositingPass();
    SlateRenderPass = new FSlateRenderPass();

    StaticMeshRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    WorldBillboardRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    EditorBillboardRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    GizmoRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    UpdateLightBufferPass->Initialize(BufferManager, Graphics, ShaderManager);
    LineRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    FogRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    EditorRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    
    CompositingPass->Initialize(BufferManager, Graphics, ShaderManager);
    PostProcessCompositingPass->Initialize(BufferManager, Graphics, ShaderManager);
    
    SlateRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
}

void FRenderer::Release()
{
    delete ShaderManager;

    delete StaticMeshRenderPass;
    delete WorldBillboardRenderPass;
    delete EditorBillboardRenderPass;
    delete GizmoRenderPass;
    delete UpdateLightBufferPass;
    delete LineRenderPass;
    delete FogRenderPass;
    delete CompositingPass;
    delete PostProcessCompositingPass;
    delete SlateRenderPass;
}

//------------------------------------------------------------------------------
// 사용하는 모든 상수 버퍼 생성
//------------------------------------------------------------------------------
void FRenderer::CreateConstantBuffers()
{
    UINT ObjectBufferSize = sizeof(FObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FObjectConstantBuffer>("FObjectConstantBuffer", nullptr, ObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, CameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT SubUVBufferSize = sizeof(FSubUVConstant);
    BufferManager->CreateBufferGeneric<FSubUVConstant>("FSubUVConstant", nullptr, SubUVBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT MaterialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, MaterialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT SubMeshBufferSize = sizeof(FSubMeshConstants);
    BufferManager->CreateBufferGeneric<FSubMeshConstants>("FSubMeshConstants", nullptr, SubMeshBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT TextureBufferSize = sizeof(FTextureUVConstants);
    BufferManager->CreateBufferGeneric<FTextureUVConstants>("FTextureConstants", nullptr, TextureBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    UINT LitUnlitBufferSize = sizeof(FLitUnlitConstants);
    BufferManager->CreateBufferGeneric<FLitUnlitConstants>("FLitUnlitConstants", nullptr, LitUnlitBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ViewModeBufferSize = sizeof(FViewModeConstants);
    BufferManager->CreateBufferGeneric<FViewModeConstants>("FViewModeConstants", nullptr, ViewModeBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ScreenConstantsBufferSize = sizeof(FScreenConstants);
    BufferManager->CreateBufferGeneric<FScreenConstants>("FScreenConstants", nullptr, ScreenConstantsBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FogConstantBufferSize = sizeof(FFogConstants);
    BufferManager->CreateBufferGeneric<FFogConstants>("FFogConstants", nullptr, FogConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT LightInfoBufferSize = sizeof(FLightInfoBuffer);
    BufferManager->CreateBufferGeneric<FLightInfoBuffer>("FLightInfoBuffer", nullptr, LightInfoBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CameraConstantLightViewBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantLightViewBuffer", nullptr, CameraConstantLightViewBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    // TODO: 함수로 분리
    ID3D11Buffer* ObjectBuffer = BufferManager->GetConstantBuffer(TEXT("FObjectConstantBuffer"));
    ID3D11Buffer* CameraConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FCameraConstantBuffer"));
    Graphics->DeviceContext->VSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->VSSetConstantBuffers(13, 1, &CameraConstantBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(13, 1, &CameraConstantBuffer);
}

void FRenderer::ReleaseConstantBuffer()
{
    BufferManager->ReleaseConstantBuffer();
}

void FRenderer::CreateCommonShader()
{
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", ShaderManager->StaticMeshLayoutDesc, ARRAYSIZE(ShaderManager->StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }

    hr = ShaderManager->AddVertexShaderAndInputLayout(L"ShaderMapVertexShader", L"Shaders/ShadowMapVertexShader.hlsl", "mainVS", ShaderManager->StaticMeshLayoutDesc, ARRAYSIZE(ShaderManager->StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    
#pragma region UberShader
    D3D_SHADER_MACRO DefinesGouraud[] =
    {
        { GOURAUD, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddVertexShaderAndInputLayout(L"GOURAUD_StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", ShaderManager->StaticMeshLayoutDesc, ARRAYSIZE(ShaderManager->StaticMeshLayoutDesc), DefinesGouraud);
    if (FAILED(hr))
    {
        return;
    }
#pragma endregion UberShader
}

void FRenderer::PrepareRender(FViewportResource* ViewportResource)
{
    // Setup Viewport
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    ViewportResource->ClearRenderTargets(Graphics->DeviceContext);

    PrepareRenderPass();
}

void FRenderer::PrepareRenderPass()
{
    StaticMeshRenderPass->PrepareRender();
    GizmoRenderPass->PrepareRender();
    WorldBillboardRenderPass->PrepareRender();
    EditorBillboardRenderPass->PrepareRender();
    UpdateLightBufferPass->PrepareRender();
    FogRenderPass->PrepareRender();
    EditorRenderPass->PrepareRender();
}

void FRenderer::ClearRenderArr()
{
    StaticMeshRenderPass->ClearRenderArr();
    WorldBillboardRenderPass->ClearRenderArr();
    EditorBillboardRenderPass->ClearRenderArr();
    GizmoRenderPass->ClearRenderArr();
    UpdateLightBufferPass->ClearRenderArr();
    FogRenderPass->ClearRenderArr();
    EditorRenderPass->ClearRenderArr();
}

void FRenderer::UpdateCommonBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FCameraConstantBuffer CameraConstantBuffer;
    CameraConstantBuffer.ViewMatrix = Viewport->GetViewMatrix();
    CameraConstantBuffer.InvViewMatrix = FMatrix::Inverse(CameraConstantBuffer.ViewMatrix);
    CameraConstantBuffer.ProjectionMatrix = Viewport->GetProjectionMatrix();
    CameraConstantBuffer.InvProjectionMatrix = FMatrix::Inverse(CameraConstantBuffer.ProjectionMatrix);
    CameraConstantBuffer.ViewLocation = Viewport->GetCameraLocation();
    CameraConstantBuffer.NearClip = Viewport->GetCameraLearClip();
    CameraConstantBuffer.FarClip = Viewport->GetCameraFarClip();
    BufferManager->UpdateConstantBuffer("FCameraConstantBuffer", CameraConstantBuffer);
}

void FRenderer::BeginRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }

    UpdateCommonBuffer(Viewport);
    
    PrepareRender(ViewportResource);
}


void FRenderer::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    BeginRender(Viewport);

    /**
     * 각 렌더 패스의 시작과 끝은 필요한 리소스를 바인딩하고 해제하는 것까지입니다.
     * 다음에 작동할 렌더 패스에서는 이전에 사용했던 리소스들을 충돌 없이 바인딩 할 수 있어야 한다는 의미입니다.
     * e.g.
     *   1번 렌더 패스: 여기에서 사용했던 RTV를 마지막에 해제함으로써, 해당 RTV와 연결된 텍스처를 쉐이더 리소스로 사용할 수 있습니다.
     *   2번 렌더 패스: 1번 렌더 패스에서 렌더한 결과 텍스처를 쉐이더 리소스로 사용할 수 있습니다.
     *
     * 경우에 따라(연속적인 렌더 패스에서 동일한 리소스를 사용하는 경우) 바인딩 및 해제 작업을 생략하는 것도 가능하지만,
     * 다음 전제 조건을 지켜주어야 합니다.
     *   1. 렌더 패스는 엄격하게 순차적으로 실행됨
     *   2. 렌더 타겟의 생명주기와 용도가 명확함
     *   3. RTV -> SRV 전환 타이밍이 정확히 지켜짐
     */
    RenderWorldScene(Viewport);
    RenderPostProcess(Viewport);
    RenderEditorOverlay(Viewport);

    // Compositing: 위에서 렌더한 결과들을 하나로 합쳐서 뷰포트의 최종 이미지를 만드는 작업
    CompositingPass->Render(Viewport);

    EndRender();
}

void FRenderer::EndRender()
{
    ClearRenderArr();
    ShaderManager->ReloadAllShaders(); // 
}

void FRenderer::RenderWorldScene(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    
    if (ShowFlag & EEngineShowFlags::SF_Primitives)
    {
        UpdateLightBufferPass->Render(Viewport);
        StaticMeshRenderPass->Render(Viewport);
    }
    
    // Render World Billboard
    if (ShowFlag & EEngineShowFlags::SF_BillboardText)
    {
        WorldBillboardRenderPass->Render(Viewport);
    }
}

void FRenderer::RenderPostProcess(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    
    if (ViewMode >= EViewModeIndex::VMI_Unlit)
    {
        return;
    }
    
    if (ShowFlag & EEngineShowFlags::SF_Fog)
    {
        FogRenderPass->Render(Viewport);
        /**
         * TODO: Fog 렌더 작업 해야 함.
         * 여기에서는 씬 렌더가 적용된 뎁스 스텐실 뷰를 SRV로 전달하고, 뎁스 스텐실 뷰를 아래에서 다시 써야함.
         */
    }

    // TODO: 포스트 프로세스 별로 각자의 렌더 타겟 뷰에 렌더하기

    /**
     * TODO: 반드시 씬에 먼저 반영되어야 하는 포스트 프로세싱 효과는 먼저 씬에 반영하고,
     *       그 외에는 렌더한 포스트 프로세싱 효과들을 이 시점에서 하나로 합친 후에, 다음에 올 컴포짓 과정에서 합성.
     */ 

    PostProcessCompositingPass->Render(Viewport);
    
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FRenderer::RenderEditorOverlay(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    
    if (GEngine->ActiveWorld->WorldType != EWorldType::Editor)
    {
        return;
    }
    
    // Render Editor Billboard
    /**
     * TODO: 에디터 전용 빌보드는 이런 방식 처럼 빌보드의 bool값을 바꿔서 렌더하기 보다는
     *       빌보드가 나와야 하는 컴포넌트는 텍스처만 가지고있고, 쉐이더를 통해 쿼드를 생성하고
     *       텍스처를 전달해서 렌더하는 방식이 더 좋음.
     *       이렇게 하는 경우 필요없는 빌보드 컴포넌트가 아웃라이너에 나오지 않음.
     */
    if (ShowFlag & EEngineShowFlags::SF_BillboardText)
    {
        EditorBillboardRenderPass->Render(Viewport);
    }

    EditorRenderPass->Render(Viewport); // TODO: 임시로 이전에 작성되었던 와이어 프레임 렌더 패스로, 이후 개선 필요.

    LineRenderPass->Render(Viewport); // 기존 뎁스를 그대로 사용하지만 뎁스를 클리어하지는 않음
    
    GizmoRenderPass->Render(Viewport); // 기존 뎁스를 SRV로 전달해서 샘플 후 비교하기 위해 기즈모 전용 DSV 사용

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FRenderer::RenderViewport(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    SlateRenderPass->Render(Viewport);
}
void FRenderer::UpdateAllShader() {
    WorldBillboardRenderPass->OnShaderReload();
    EditorBillboardRenderPass->OnShaderReload();
    GizmoRenderPass->OnShaderReload();
    LineRenderPass->OnShaderReload();
    FogRenderPass->OnShaderReload();
}
