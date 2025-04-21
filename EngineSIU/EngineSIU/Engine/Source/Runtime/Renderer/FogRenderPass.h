#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <memory>

#include "IRenderPass.h"
#include "Engine/Classes/Components/HeightFogComponent.h"

class FGraphicsDevice;
class FDXDShaderManager;
class FDXDBufferManager;
class FEditorViewportClient;

class FFogRenderPass : public IRenderPass
{
public:
    FFogRenderPass();
    virtual ~FFogRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    
    virtual void PrepareRender() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;
    

    // Fog 렌더링용 셰이더 생성 및 입력 레이아웃 설정
    void CreateShader();
    void ReleaseShader();

    void OnShaderReload();

    void PrepareRenderState();

    void UpdateFogConstant(UHeightFogComponent* Fog);

    void CreateBlendState();
    void CreateSampler();

private:
    FGraphicsDevice* Graphics = nullptr;
    FDXDBufferManager* BufferManager = nullptr;
    FDXDShaderManager* ShaderManager = nullptr;
    
    ID3D11SamplerState* Sampler = nullptr;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    ID3D11BlendState* BlendState = nullptr;

    TArray<UHeightFogComponent*> FogComponents;
};
