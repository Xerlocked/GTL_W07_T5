#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>

#include "EngineBaseTypes.h"
#include "Define.h"
#include "Container/Set.h"

#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDBufferManager.h"


class FPostProcessCompositingPass;
enum class EResourceType : uint8;

class FSceneRenderPass;
class UWorld;
class UObject;

class FDXDShaderManager;
class FEditorViewportClient;

class FViewportResource;

class FStaticMeshRenderPass;
class FWorldBillboardRenderPass;
class FEditorBillboardRenderPass;
class FGizmoRenderPass;
class FUpdateLightBufferPass;
class FDepthBufferDebugPass;
class FWorldNormalDebugPass;
class FLineRenderPass;
class FFogRenderPass;
class FCompositingPass;
class FSlateRenderPass;
class FEditorRenderPass;

class FRenderer
{
public:
    //==========================================================================
    // 초기화/해제 관련 함수
    //==========================================================================
    void Initialize(FGraphicsDevice* graphics, FDXDBufferManager* bufferManager);
    void Release();

    //==========================================================================
    // 렌더 패스 관련 함수
    //==========================================================================
    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderViewport(const std::shared_ptr<FEditorViewportClient>& Viewport); // TODO: 추후 RenderSlate로 변경해야함

protected:
    void BeginRender(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void UpdateCommonBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void PrepareRender(FViewportResource* ViewportResource);
    void PrepareRenderPass();
    void RenderWorldScene(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderPostProcess(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderEditorOverlay(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void EndRender();
    void ClearRenderArr();
    
    //==========================================================================
    // 버퍼 생성/해제 함수 (템플릿 포함)
    //==========================================================================
public:
    template<typename T>
    ID3D11Buffer* CreateImmutableVertexBuffer(const FString& key, const TArray<T>& Vertices);

    ID3D11Buffer* CreateImmutableIndexBuffer(const FString& key, const TArray<uint32>& indices);
    
    // 상수 버퍼 생성/해제
    void CreateConstantBuffers();
    void ReleaseConstantBuffer();

    void CreateCommonShader();

public:
   
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager = nullptr;

    FStaticMeshRenderPass* StaticMeshRenderPass = nullptr;
    FWorldBillboardRenderPass* WorldBillboardRenderPass = nullptr;
    FEditorBillboardRenderPass* EditorBillboardRenderPass = nullptr;
    FGizmoRenderPass* GizmoRenderPass = nullptr;
    FUpdateLightBufferPass* UpdateLightBufferPass = nullptr;
    FLineRenderPass* LineRenderPass = nullptr;
    FFogRenderPass* FogRenderPass = nullptr;
    FEditorRenderPass* EditorRenderPass = nullptr;
    
    FCompositingPass* CompositingPass = nullptr;
    FPostProcessCompositingPass* PostProcessCompositingPass = nullptr;
    
    FSlateRenderPass* SlateRenderPass = nullptr;
    void UpdateAllShader();
};

template<typename T>
inline ID3D11Buffer* FRenderer::CreateImmutableVertexBuffer(const FString& key, const TArray<T>& Vertices)
{
    FVertexInfo VertexBufferInfo;
    BufferManager->CreateVertexBuffer(key, Vertices, VertexBufferInfo);
    return VertexBufferInfo.VertexBuffer;
}

inline ID3D11Buffer* FRenderer::CreateImmutableIndexBuffer(const FString& key, const TArray<uint32>& indices)
{
    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(key, indices, IndexInfo);
    return IndexInfo.IndexBuffer;
}
