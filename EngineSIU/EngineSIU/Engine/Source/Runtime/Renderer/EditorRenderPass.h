#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include "Math/Matrix.h"
#include "Container/Set.h"
#include "RenderResources.h"

class FDXDBufferManager;
class FGraphicsDevice;
class UWorld;
class FEditorViewportClient;
class FDXDShaderManager;

class FEditorRenderPass
{
public:
    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager);
    void Render(std::shared_ptr<FEditorViewportClient> Viewport);
    void Release();

    void PrepareRender();

    void ClearRenderArr();

private:
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
    FRenderResourcesDebug Resources;

    void CreateShaders();
    void PrepareShader(FShaderResource ShaderResource) const;
    void ReleaseShaders();

    void CreateBuffers();
    void CreateConstantBuffers();

    void LazyLoad();
    
    void PrepareRendertarget(std::shared_ptr<FEditorViewportClient> Viewport);

    void PrepareConstantbufferGlobal();
    void UpdateConstantbufferGlobal(FConstantBufferCamera Buffer);

    // Gizmo 관련 함수
    //void RenderGizmos(const UWorld* World);
    void PrepareShaderGizmo();
    void PrepareConstantbufferGizmo();

    // Axis
    void RenderAxis();

    // AABB
    //void RenderAABBInstanced(const UWorld* World);
    void PrepareConstantbufferAABB();
    void UdpateConstantbufferAABBInstanced(TArray<FConstantBufferDebugAABB> Buffer);

    // Sphere
    void RenderPointlightInstanced();
    void PrepareConstantbufferPointlight();
    void UdpateConstantbufferPointlightInstanced(TArray<FConstantBufferDebugSphere> Buffer);
     
    // Cone
    void RenderSpotlightInstanced();
    void PrepareConstantbufferSpotlight();
    void UdpateConstantbufferSpotlightInstanced(TArray<FConstantBufferDebugCone> Buffer);

    // Grid
    // void RenderGrid(std::shared_ptr<FEditorViewportClient> ActiveViewport);
    // void PrepareConstantbufferGrid();
    // void UpdateConstantbufferGrid(FConstantBufferDebugGrid Buffer);

    // Icon
    void RenderIcons(const UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void PrepareConstantbufferIcon();
    void UdpateConstantbufferIcon(FConstantBufferDebugIcon Buffer);
    void UpdateTextureIcon(IconType type);

    // Arrow
    void RenderArrows();
    void PrepareConstantbufferArrow();
    void UdpateConstantbufferArrow(FConstantBufferDebugArrow Buffer);

    // ShaderManager의 Hot Reload에 대응하기 위한 함수
    void SetShaderAndPrepare(const std::wstring& VertexKey, const std::wstring& PixelKey, FShaderResource& ShaderSlot);

    const UINT32 ConstantBufferSizeAABB = 8;
    const UINT32 ConstantBufferSizeSphere = 8;
    const UINT32 ConstantBufferSizeCone = 2; // 최대

    const uint32 NumSegments = 16;
};

