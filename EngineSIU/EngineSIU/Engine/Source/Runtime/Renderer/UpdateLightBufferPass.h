#pragma once

#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"

class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

class UPointLightComponent;
class USpotLightComponent;
class UDirectionalLightComponent;
class UAmbientLightComponent;
class UStaticMeshComponent;

class FUpdateLightBufferPass : public IRenderPass
{
public:
    FUpdateLightBufferPass();
    virtual ~FUpdateLightBufferPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    void PrepareRenderState();
    virtual void PrepareRender() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;
    void OnShaderReload();
    void UpdateLightBuffer() const;

    void BakeShadowMap(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void CreateShader();
    void ReleaseShader();
    void RenderPrimitive(OBJ::FStaticMeshRenderData* RenderData) const;
    void UpdateObjectConstant(const FMatrix& WorldMatrix) const;

private:
    TArray<USpotLightComponent*> SpotLights;
    TArray<UPointLightComponent*> PointLights;
    TArray<UDirectionalLightComponent*> DirectionalLights;
    TArray<UAmbientLightComponent*> AmbientLights;
    TArray<UStaticMeshComponent*> StaticMeshComponents;

    D3D11_VIEWPORT ShadowViewport;
    D3D11_VIEWPORT PointShadowViewport;

    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader* PixelShader;
    ID3D11InputLayout* InputLayout;

    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;       
};
