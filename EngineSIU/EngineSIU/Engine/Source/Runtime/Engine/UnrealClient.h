#pragma once
#include "Define.h" 
#include <d3d11.h>

#include "Container/Map.h"

class FViewportResource;

enum class EViewScreenLocation : uint8
{
    EVL_TopLeft,
    EVL_TopRight,
    EVL_BottomLeft,
    EVL_BottomRight,
    EVL_MAX,
};

enum class EResourceType : uint8
{
    ERT_Compositing,
    ERT_Scene,
    ERT_PP_Fog,
    ERT_Editor,
    ERT_Overlay,
    ERT_PostProcessCompositing,
    ERT_MAX,
};

struct FRenderTargetRHI
{
    ID3D11Texture2D* Texture2D = nullptr;
    ID3D11RenderTargetView* RTV = nullptr;
    ID3D11ShaderResourceView* SRV = nullptr;

    void Release()
    {
        if (SRV)
        {
            SRV->Release();
            SRV = nullptr;
        }
        if (RTV)
        {
            RTV->Release();
            RTV = nullptr;
        }
        if (Texture2D)
        {
            Texture2D->Release();
            Texture2D = nullptr;
        }
    }
};

class FViewportResource
{
public:
    FViewportResource();
    ~FViewportResource();

    void Initialize(uint32 InWidth, uint32 InHeight);
    void Resize(uint32 NewWidth, uint32 NewHeight);

    void Release();

    HRESULT CreateResource(EResourceType Type);
    
    D3D11_VIEWPORT& GetD3DViewport() { return D3DViewport; }
    
    ID3D11Texture2D*& GetDepthStencilTexture() { return DepthStencilTexture; }
    ID3D11DepthStencilView*& GetDepthStencilView() { return DepthStencilView; }
    ID3D11ShaderResourceView*& GetDepthStencilSRV() { return DepthStencilSRV; }

    ID3D11Texture2D*& GetDirectionalShadowMapTexture(){ return DirectionalShadowMapTexture; }
    ID3D11DepthStencilView*& GetDirectionalShadowMapDSV() { return DirectionalShadowMapDSV; }
    ID3D11ShaderResourceView*& GetDirectionalShadowMapSRV() { return DirectionalShadowMapSRV; }

    ID3D11Texture2D*& GetSpotShadowMapTexture(){ return SpotShadowMapTexture; }
    ID3D11DepthStencilView*& GetSpotShadowMapDSV() { return SpotShadowMapDSV; }
    ID3D11ShaderResourceView*& GetSpotShadowMapSRV() { return SpotShadowMapSRV; }

    ID3D11Texture2D*& GetGizmoDepthStencilTexture() { return GizmoDepthStencilTexture; }
    ID3D11DepthStencilView*& GetGizmoDepthStencilView() { return GizmoDepthStencilView; }

    TMap<EResourceType, FRenderTargetRHI>& GetRenderTargets();

    // 해당 타입의 리소스를 리턴. 없는 경우에는 생성해서 리턴.
    FRenderTargetRHI* GetRenderTarget(EResourceType Type);

    bool HasRenderTarget(EResourceType Type) const;

    // 가지고있는 모든 리소스의 렌더 타겟 뷰를 clear
    void ClearRenderTargets(ID3D11DeviceContext* DeviceContext);

    // 지정한 타입의 렌더 타겟 뷰를 clear. 없는 경우 생성해서 clear.
    void ClearRenderTarget(ID3D11DeviceContext* DeviceContext, EResourceType Type);

    std::array<float, 4> GetClearColor(EResourceType Type) const;
    
private:
    // DirectX
    D3D11_VIEWPORT D3DViewport = {};

    ID3D11Texture2D* DepthStencilTexture = nullptr;
    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11ShaderResourceView* DepthStencilSRV = nullptr;

    ID3D11Texture2D* DirectionalShadowMapTexture = nullptr;
    ID3D11DepthStencilView* DirectionalShadowMapDSV = nullptr;
    ID3D11ShaderResourceView* DirectionalShadowMapSRV = nullptr;

    ID3D11Texture2D* SpotShadowMapTexture = nullptr;
    ID3D11DepthStencilView* SpotShadowMapDSV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapSRV = nullptr;
    
    // int DirectionalShadowMapWidth = 4096;
    // int DirectionalShadowMapHeight = 4096;
    int ShadowMapWidth = 1024;
    int ShadowMapHeight = 1024;
    
    ID3D11Texture2D* GizmoDepthStencilTexture = nullptr;
    ID3D11DepthStencilView* GizmoDepthStencilView = nullptr;

    TMap<EResourceType, FRenderTargetRHI> RenderTargets;

    HRESULT CreateDepthStencilResources();

    void ReleaseDepthStencilResources();
    void ReleaseResources();
    void ReleaseResource(EResourceType Type);

    /**
     * ClearColors 맵에는 모든 EResourceType에 대응하는 색상을
     * 이 클래스의 생성자에서 반드시 추가해야 함.
     */
    TMap<EResourceType, std::array<float, 4>> ClearColors;
};


class FViewport
{
public:
    FViewport();
    FViewport(EViewScreenLocation InViewLocation);
    ~FViewport();

    void Initialize(const FRect& InRect);
    void ResizeViewport(const FRect& InRect);
    void ResizeViewport(const FRect& Top, const FRect& Bottom, const FRect& Left, const FRect& Right);

    D3D11_VIEWPORT& GetD3DViewport() const { return ViewportResource->GetD3DViewport(); }

    EViewScreenLocation GetViewLocation() const { return ViewLocation; }

    FViewportResource* GetViewportResource() const { return ViewportResource; }

    FRect GetRect() const { return Rect; }

    bool bIsHovered(const FVector2D& InPoint) const;

private:
    FViewportResource* ViewportResource;

    EViewScreenLocation ViewLocation;   // 뷰포트 위치

    // 이 값은 화면의 크기 뿐만 아니라 위치 정보도 가지고 있음.
    FRect Rect;
};
