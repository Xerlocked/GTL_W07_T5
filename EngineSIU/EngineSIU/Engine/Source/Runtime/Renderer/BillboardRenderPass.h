#pragma once

#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"

enum class EResourceType : uint8;
class UBillboardComponent;
class FDXDBufferManager;
class FGraphicsDevice;
class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

class FBillboardRenderPass : public IRenderPass
{
public:
    FBillboardRenderPass();
    virtual ~FBillboardRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;

    virtual void PrepareRender() override;
    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void SetupVertexBuffer(ID3D11Buffer* pVertexBuffer, UINT numVertices) const;
    // Texture 셰이더 관련
    void PrepareTextureShader() const;
    void PrepareSubUVConstant() const;

    // 상수 버퍼 업데이트 함수
    void UpdateSubUVConstant(FVector2D uvOffset, FVector2D uvScale) const;
    
    // Primitive 드로우 함수
    void RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11Buffer* pIndexBuffer, UINT numIndices,
        ID3D11ShaderResourceView* _TextureSRV, ID3D11SamplerState* _SamplerState) const;

    void RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11ShaderResourceView* _TextureSRV, ID3D11SamplerState* _SamplerState) const;

    void CreateShader();
    void ReleaseShader();
    void OnShaderReload();

protected:
    TArray<UBillboardComponent*> BillboardComps;

    EResourceType ResourceType;

private:
    ID3D11VertexShader* VertexShader;
    
    ID3D11PixelShader* PixelShader;
    
    ID3D11InputLayout* InputLayout;

    FDXDBufferManager* BufferManager;
    
    FGraphicsDevice* Graphics;
    
    FDXDShaderManager* ShaderManager;

};
