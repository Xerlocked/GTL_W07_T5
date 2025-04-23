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
    ID3D11RenderTargetView*& GetDirectionalShadowMapRTV() { return DirectionalShadowMapRTV; }
    ID3D11ShaderResourceView*& GetDirectionalShadowMapSRV() { return DirectionalShadowMapSRV; }
    ID3D11ShaderResourceView*& GetDirectionalShadowMapCompareSRV() { return DirectionalShadowMapCompareSRV; }

    ID3D11Texture2D*& GetDirectionalShadowMapDepthTexture(){ return DirectionalShadowMapDepthTexture; }
    ID3D11DepthStencilView*& GetDirectionalShadowMapDSV() { return DirectionalShadowMapDSV; }
    
    ID3D11Texture2D*& GetSpotShadowMapTexture(){ return SpotShadowMapTexture; }
    ID3D11RenderTargetView*& GetSpotShadowMapRTV() { return SpotShadowMapRTV; }
    ID3D11ShaderResourceView*& GetSpotShadowMapSRV() { return SpotShadowMapSRV; }
    ID3D11ShaderResourceView*& GetSpotShadowMapCompareSRV() { return SpotShadowMapCompareSRV; }

    ID3D11Texture2D*& GetSpotShadowMapDepthTexture(){ return SpotShadowMapDepthTexture; }
    ID3D11DepthStencilView*& GetSpotShadowMapDSV() { return SpotShadowMapDSV; }
    
    ID3D11ShaderResourceView*& GetPointShadowMapArraySRV() { return PointShadowMapArraySRV;}
    ID3D11Texture2D*& GetPointShadowMapArrayTexture() { return PointShadowMapArrayTexture; } 

    ID3D11RenderTargetView*& GetPointShadowMapRTV(int i) { return PointShadowMapRTVs[i];}
    ID3D11DepthStencilView*& GetPointShadowMapDSV(int i) { return PointLightDSVs[i];}

    ID3D11Texture2D*& GetGizmoDepthStencilTexture() { return GizmoDepthStencilTexture; }
    ID3D11DepthStencilView*& GetGizmoDepthStencilView() { return GizmoDepthStencilView; }

    ID3D11ShaderResourceView* GetPointShadowMapFaceSRV(uint32 FaceIndex) const
    {
        if (FaceIndex >= 6 * MAX_POINT_LIGHT) return nullptr;
        return PointShadowMapFaceSRVs[FaceIndex];
    }



    TMap<EResourceType, FRenderTargetRHI>& GetRenderTargets();



    // 해당 타입의 리소스를 리턴. 없는 경우에는 생성해서 리턴.
    FRenderTargetRHI* GetRenderTarget(EResourceType Type);

    bool HasRenderTarget(EResourceType Type) const;

    // 가지고있는 모든 리소스의 렌더 타겟 뷰를 clear
    void ClearRenderTargets(ID3D11DeviceContext* DeviceContext);

    // 지정한 타입의 렌더 타겟 뷰를 clear. 없는 경우 생성해서 clear.
    void ClearRenderTarget(ID3D11DeviceContext* DeviceContext, EResourceType Type);

    std::array<float, 4> GetClearColor(EResourceType Type) const;
    HRESULT CreateShadowMapResources();

    // inline static int ShadowMapWidth = 1024;
    // inline static int ShadowMapHeight = 1024;
    inline static int ShadowMapWidth = 4096;
    inline static int ShadowMapHeight = 4096;
    
private:
    // DirectX
    D3D11_VIEWPORT D3DViewport = {};

    ID3D11Texture2D* DepthStencilTexture = nullptr;
    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11ShaderResourceView* DepthStencilSRV = nullptr;

    ID3D11Texture2D* DirectionalShadowMapTexture = nullptr;
    ID3D11RenderTargetView* DirectionalShadowMapRTV = nullptr;
    ID3D11ShaderResourceView* DirectionalShadowMapSRV = nullptr;
    ID3D11ShaderResourceView* DirectionalShadowMapCompareSRV = nullptr;

    ID3D11Texture2D* DirectionalShadowMapDepthTexture = nullptr;
    ID3D11DepthStencilView* DirectionalShadowMapDSV = nullptr;
    
    ID3D11Texture2D* SpotShadowMapTexture = nullptr;
    ID3D11RenderTargetView* SpotShadowMapRTV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapSRV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapCompareSRV = nullptr;

    ID3D11Texture2D* SpotShadowMapDepthTexture = nullptr;
    ID3D11DepthStencilView* SpotShadowMapDSV = nullptr;

    ID3D11Texture2D* PointShadowMapArrayTexture = nullptr; // 큐브맵 배열 텍스처
    ID3D11RenderTargetView* PointShadowMapRTVs[MAX_POINT_LIGHT * 6] = {}; 
    ID3D11ShaderResourceView* PointShadowMapArraySRV = nullptr; // 전체 배열 SRV
    ID3D11ShaderResourceView* PointShadowMapFaceSRVs[MAX_POINT_LIGHT * 6] = {}; // 면별 SRV

    ID3D11Texture2D* PointShadowMapArrayDepthTexture = nullptr;
    ID3D11DepthStencilView* PointLightDSVs[MAX_POINT_LIGHT * 6] = {}; // 라이트별 DSV
    
    ID3D11Texture2D* GizmoDepthStencilTexture = nullptr;
    ID3D11DepthStencilView* GizmoDepthStencilView = nullptr;



    
    
    
    TMap<EResourceType, FRenderTargetRHI> RenderTargets;

    HRESULT CreateDepthStencilResources();
    void ReleaseShadowMapResources();

    HRESULT CreateCubeShadowMapResources();

    HRESULT CreatePointShadowMapFaceSRVs();

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
