#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"

class FDXDBufferManager;
class FGraphicsDevice;
class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

struct FLinePrimitiveBatchArgs;

class FLineRenderPass : public IRenderPass
{
public:
    FLineRenderPass();
    ~FLineRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRender() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;

    // 라인 렌더링 전용 함수
    void CreateShader();
    void OnShaderReload();

    void PrepareLineShader() const;
    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;
    void ProcessLineRendering(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void DrawLineBatch(const FLinePrimitiveBatchArgs& BatchArgs) const;

private:
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    // 라인 셰이더 관련 멤버
    ID3D11VertexShader* VertexLineShader;
    ID3D11PixelShader* PixelLineShader;
};
