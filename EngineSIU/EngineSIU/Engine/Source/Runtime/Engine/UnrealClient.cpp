#include "UnrealClient.h"

#include "EngineLoop.h"
#include <array>

FViewportResource::FViewportResource()
{
    ClearColors.Add(EResourceType::ERT_Compositing, { 0.f, 0.f, 0.f, 1.f });
    ClearColors.Add(EResourceType::ERT_Scene,  { 0.025f, 0.025f, 0.025f, 1.0f });
    ClearColors.Add(EResourceType::ERT_PP_Fog, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Editor, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Overlay, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_PostProcessCompositing, { 0.f, 0.f, 0.f, 0.f });
}

FViewportResource::~FViewportResource()
{
    Release();
    ReleaseShadowMapResources();
    ClearColors.Empty();
}

void FViewportResource::Initialize(uint32 InWidth, uint32 InHeight)
{
    D3DViewport.TopLeftX = 0.f;
    D3DViewport.TopLeftY = 0.f;
    D3DViewport.Height = static_cast<float>(InHeight);
    D3DViewport.Width = static_cast<float>(InWidth);
    D3DViewport.MaxDepth = 1.0f;
    D3DViewport.MinDepth = 0.0f;

    HRESULT hr = S_OK;

    hr = CreateDepthStencilResources();
    if (FAILED(hr))
    {
        return;
    }

    hr = CreateShadowMapResources();
    if (FAILED(hr))
    {
        return;
    }
    
    // Essential resources
    hr = CreateResource(EResourceType::ERT_Compositing);
    if (FAILED(hr))
    {
        return;
    }

    hr = CreateResource(EResourceType::ERT_Scene);
    if (FAILED(hr))
    {
        return;
    }
}

void FViewportResource::Resize(uint32 NewWidth, uint32 NewHeight)
{
    Release();

    D3DViewport.Height = static_cast<float>(NewHeight);
    D3DViewport.Width = static_cast<float>(NewWidth);

    HRESULT hr = S_OK;
    hr = CreateDepthStencilResources();
    if (FAILED(hr))
    {
        return;
    }

    for (auto& [Type, Resource] : RenderTargets)
    {
        CreateResource(Type);
    }
}

void FViewportResource::Release()
{
    ReleaseDepthStencilResources();
    ReleaseResources();
}

HRESULT FViewportResource::CreateResource(EResourceType Type)
{
    if (HasRenderTarget(Type))
    {
        ReleaseResource(Type);
    }
    
    FRenderTargetRHI NewResource;
    
    HRESULT hr = S_OK;
    
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = static_cast<uint32>(D3DViewport.Width);
    TextureDesc.Height = static_cast<uint32>(D3DViewport.Height);
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    NewResource.Texture2D = FEngineLoop::GraphicDevice.CreateTexture2D(TextureDesc, nullptr);

    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: srgb 옵션 고려해보기
    RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(NewResource.Texture2D, &RTVDesc, &NewResource.RTV);
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TextureDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(NewResource.Texture2D, &SRVDesc, &NewResource.SRV);
    if (FAILED(hr))
    {
        return hr;
    }

    RenderTargets.Add(Type, NewResource);

    return hr;
}

bool FViewportResource::HasRenderTarget(EResourceType Type) const
{
    return RenderTargets.Contains(Type);
}

TMap<EResourceType, FRenderTargetRHI>& FViewportResource::GetRenderTargets()
{
    return RenderTargets;
}

FRenderTargetRHI* FViewportResource::GetRenderTarget(EResourceType Type)
{
    if (!RenderTargets.Contains(Type))
    {
        if (FAILED(CreateResource(Type)))
        {
            return nullptr;
        }
    }
    return RenderTargets.Find(Type);
}

void FViewportResource::ClearRenderTargets(ID3D11DeviceContext* DeviceContext)
{
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (auto& [Type, Resource] : RenderTargets)
    {
        DeviceContext->ClearRenderTargetView(Resource.RTV, ClearColors[Type].data());
    }

    /////////////////////////////////////
    /// ShadowMap
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    DeviceContext->ClearRenderTargetView(DirectionalShadowMapRTV, ClearColor);
    DeviceContext->ClearRenderTargetView(SpotShadowMapRTV, ClearColor);

    DeviceContext->ClearDepthStencilView(DirectionalShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    DeviceContext->ClearDepthStencilView(SpotShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void FViewportResource::ClearRenderTarget(ID3D11DeviceContext* DeviceContext, EResourceType Type)
{
    if (FRenderTargetRHI* Resource = GetRenderTarget(Type))
    {
        DeviceContext->ClearRenderTargetView(Resource->RTV, ClearColors[Type].data());
    }
}

std::array<float, 4> FViewportResource::GetClearColor(EResourceType Type) const
{
    if (const std::array<float, 4>* Found = ClearColors.Find(Type))
    {
        return *Found;
    }
    return { 0.0f, 0.0f, 0.0f, 1.0f };
}

HRESULT FViewportResource::CreateShadowMapResources()
{
    D3D11_TEXTURE2D_DESC ShadowMapTextureDesc;
    ZeroMemory(&ShadowMapTextureDesc, sizeof(D3D11_TEXTURE2D_DESC));
    ShadowMapTextureDesc.Format = DXGI_FORMAT_R32G32_TYPELESS;
    ShadowMapTextureDesc.MipLevels = 4; //밉맵 최대레벨이 4
    ShadowMapTextureDesc.ArraySize = 1;
    ShadowMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    ShadowMapTextureDesc.CPUAccessFlags = 0;
    ShadowMapTextureDesc.SampleDesc.Count = 1;
    ShadowMapTextureDesc.SampleDesc.Quality = 0;
    ShadowMapTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    ShadowMapTextureDesc.Height = (UINT)ShadowMapHeight;
    ShadowMapTextureDesc.Width = (UINT)ShadowMapWidth;
    ShadowMapTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    HRESULT hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&ShadowMapTextureDesc, nullptr, &DirectionalShadowMapTexture);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&ShadowMapTextureDesc, nullptr, &SpotShadowMapTexture);
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC ShadowShaderResourceViewDesc;
    ZeroMemory(&ShadowShaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    ShadowShaderResourceViewDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    ShadowShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShadowShaderResourceViewDesc.Texture2D.MipLevels = -1;
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(DirectionalShadowMapTexture, &ShadowShaderResourceViewDesc, &DirectionalShadowMapSRV);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(SpotShadowMapTexture, &ShadowShaderResourceViewDesc, &SpotShadowMapSRV);
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_RENDER_TARGET_VIEW_DESC ShadowMapRenderTargetViewDesc;
    ZeroMemory(&ShadowMapRenderTargetViewDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
    ShadowMapRenderTargetViewDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    ShadowMapRenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    ShadowMapRenderTargetViewDesc.Texture2D.MipSlice = 0;
    hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(DirectionalShadowMapTexture, &ShadowMapRenderTargetViewDesc, &DirectionalShadowMapRTV);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(SpotShadowMapTexture, &ShadowMapRenderTargetViewDesc, &SpotShadowMapRTV);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_TEXTURE2D_DESC ShadowMapTextureDepthDesc;
    ZeroMemory(&ShadowMapTextureDepthDesc, sizeof(D3D11_TEXTURE2D_DESC));
    ShadowMapTextureDepthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    ShadowMapTextureDepthDesc.MipLevels = 0;
    ShadowMapTextureDepthDesc.ArraySize = 1;
    ShadowMapTextureDepthDesc.Usage = D3D11_USAGE_DEFAULT;
    ShadowMapTextureDepthDesc.CPUAccessFlags = 0;
    ShadowMapTextureDepthDesc.SampleDesc.Count = 1;
    ShadowMapTextureDepthDesc.SampleDesc.Quality = 0;
    ShadowMapTextureDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ShadowMapTextureDepthDesc.Height = (UINT)ShadowMapHeight;
    ShadowMapTextureDepthDesc.Width = (UINT)ShadowMapWidth;
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&ShadowMapTextureDepthDesc, nullptr, &DirectionalShadowMapDepthTexture);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&ShadowMapTextureDepthDesc, nullptr, &SpotShadowMapDepthTexture);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC ShadowMapCompareSRVDesc;
    ZeroMemory(&ShadowMapCompareSRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    ShadowMapCompareSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    ShadowMapCompareSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShadowMapCompareSRVDesc.Texture2D.MipLevels = 1;
    
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(DirectionalShadowMapDepthTexture, &ShadowMapCompareSRVDesc, &DirectionalShadowMapCompareSRV);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(SpotShadowMapDepthTexture, &ShadowMapCompareSRVDesc, &SpotShadowMapCompareSRV);
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_DEPTH_STENCIL_VIEW_DESC ShadowMapDepthStencilViewDesc;
    ZeroMemory(&ShadowMapDepthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    ShadowMapDepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    ShadowMapDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ShadowMapDepthStencilViewDesc.Texture2D.MipSlice = 0;
    hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(DirectionalShadowMapDepthTexture, &ShadowMapDepthStencilViewDesc, &DirectionalShadowMapDSV);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(SpotShadowMapDepthTexture, &ShadowMapDepthStencilViewDesc, &SpotShadowMapDSV);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CreateCubeShadowMapResources();
    if (FAILED(hr)) return hr;
}

HRESULT FViewportResource::CreateDepthStencilResources()
{
    HRESULT hr = S_OK;
    
    D3D11_TEXTURE2D_DESC DepthStencilTextureDesc = {};
    DepthStencilTextureDesc.Width = static_cast<uint32>(D3DViewport.Width);
    DepthStencilTextureDesc.Height = static_cast<uint32>(D3DViewport.Height);
    DepthStencilTextureDesc.MipLevels = 1;
    DepthStencilTextureDesc.ArraySize = 1;
    DepthStencilTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    DepthStencilTextureDesc.SampleDesc.Count = 1;
    DepthStencilTextureDesc.SampleDesc.Quality = 0;
    DepthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    DepthStencilTextureDesc.CPUAccessFlags = 0;
    DepthStencilTextureDesc.MiscFlags = 0;
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&DepthStencilTextureDesc, nullptr, &DepthStencilTexture);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&DepthStencilTextureDesc, nullptr, &GizmoDepthStencilTexture);
    if (FAILED(hr))
    {
        return hr;
    }    

    D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
    DepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    DepthStencilViewDesc.Texture2D.MipSlice = 0;
    hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(DepthStencilTexture,  &DepthStencilViewDesc,  &DepthStencilView);
    if (FAILED(hr))
    {
        return hr;
    }
    
    hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(GizmoDepthStencilTexture,  &DepthStencilViewDesc,  &GizmoDepthStencilView);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC DepthStencilDesc = {};
    DepthStencilDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    DepthStencilDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Texture2D.MostDetailedMip = 0;
    DepthStencilDesc.Texture2D.MipLevels = 1;
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(DepthStencilTexture, &DepthStencilDesc, &DepthStencilSRV);
    if (FAILED(hr))
    {
        return hr;
    }
    
    return hr;
}

void FViewportResource::ReleaseShadowMapResources()
{
        
    if (DirectionalShadowMapTexture)
    {
        DirectionalShadowMapTexture->Release();
        DirectionalShadowMapTexture = nullptr;
    }
    if (DirectionalShadowMapRTV)
    {
        DirectionalShadowMapRTV->Release();
        DirectionalShadowMapRTV = nullptr;
    }
    if (DirectionalShadowMapSRV)
    {
        DirectionalShadowMapSRV->Release();
        DirectionalShadowMapSRV = nullptr;
    }
    if (SpotShadowMapTexture)
    {
        SpotShadowMapTexture->Release();
        SpotShadowMapTexture = nullptr;
    }
    if (SpotShadowMapRTV)
    {
        SpotShadowMapRTV->Release();
        SpotShadowMapRTV = nullptr;
    }
    if (SpotShadowMapSRV)
    {
        SpotShadowMapSRV->Release();
        SpotShadowMapSRV = nullptr;
    }

    if (DirectionalShadowMapDepthTexture)
    {
        DirectionalShadowMapDepthTexture->Release();
        DirectionalShadowMapDepthTexture = nullptr;
    }

    if (DirectionalShadowMapDSV)
    {
        DirectionalShadowMapDSV->Release();
        DirectionalShadowMapDSV = nullptr;
    }

    if (SpotShadowMapDepthTexture)
    {
        SpotShadowMapDepthTexture->Release();
        SpotShadowMapDepthTexture = nullptr;
    }

    if (SpotShadowMapDSV)
    {
        SpotShadowMapDSV->Release();
        SpotShadowMapDSV = nullptr;
    }

    if (SpotShadowMapCompareSRV)
    {
        SpotShadowMapCompareSRV->Release();
        SpotShadowMapCompareSRV = nullptr;
    }

    if (DirectionalShadowMapSRV)
    {
        DirectionalShadowMapSRV->Release();
        DirectionalShadowMapSRV = nullptr;
    }
    
    for (int i = 0; i < 6; ++i)
    {
        if (PointShadowMapFaceSRVs[i])
        {
            PointShadowMapFaceSRVs[i]->Release();
            PointShadowMapFaceSRVs[i] = nullptr;
        }
    }
    
    for (auto& dsv : PointLightDSVs) {
        if (dsv) dsv->Release();
        dsv = nullptr;
    }

    for (auto& srv : PointShadowMapFaceSRVs) {
        if (srv) srv->Release();
        srv = nullptr;
    }
}

void FViewportResource::ReleaseDepthStencilResources()
{
    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }
    if (DepthStencilSRV)
    {
        DepthStencilSRV->Release();
        DepthStencilSRV = nullptr;
    }
    if (DepthStencilTexture)
    {
        DepthStencilTexture->Release();
        DepthStencilTexture = nullptr;
    }
}

void FViewportResource::ReleaseResources()
{
    for (auto& [Type, Resource] : RenderTargets)
    {
        Resource.Release();
    }
}

void FViewportResource::ReleaseResource(EResourceType Type)
{
    if (HasRenderTarget(Type))
    {
        RenderTargets[Type].Release();
    }
}


FViewport::FViewport()
    : FViewport(EViewScreenLocation::EVL_MAX)
{
}

FViewport::FViewport(EViewScreenLocation InViewLocation)
    : ViewportResource(new FViewportResource())
    , ViewLocation(InViewLocation) 
{
}

FViewport::~FViewport()
{
    delete ViewportResource;
}

void FViewport::Initialize(const FRect& InRect)
{
    Rect = InRect;
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);

    ViewportResource->Initialize(Width, Height);
}

void FViewport::ResizeViewport(const FRect& InRect)
{
    Rect = InRect;
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);

    ViewportResource->Resize(Width, Height);
}

void FViewport::ResizeViewport(const FRect& Top, const FRect& Bottom, const FRect& Left, const FRect& Right)
{
    switch (ViewLocation)
    {
    case EViewScreenLocation::EVL_TopLeft:
        Rect.TopLeftX = Left.TopLeftX;
        Rect.TopLeftY = Top.TopLeftY;
        Rect.Width = Left.Width;
        Rect.Height = Top.Height;
        break;
    case EViewScreenLocation::EVL_TopRight:
        Rect.TopLeftX = Right.TopLeftX;
        Rect.TopLeftY = Top.TopLeftY;
        Rect.Width = Right.Width;
        Rect.Height = Top.Height;
        break;
    case EViewScreenLocation::EVL_BottomLeft:
        Rect.TopLeftX = Left.TopLeftX;
        Rect.TopLeftY = Bottom.TopLeftY;
        Rect.Width = Left.Width;
        Rect.Height = Bottom.Height;
        break;
    case EViewScreenLocation::EVL_BottomRight:
        Rect.TopLeftX = Right.TopLeftX;
        Rect.TopLeftY = Bottom.TopLeftY;
        Rect.Width = Right.Width;
        Rect.Height = Bottom.Height;
        break;
    default:
        return;
    }
    
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);
    ViewportResource->Resize(Width, Height);
}

bool FViewport::bIsHovered(const FVector2D& InPoint) const
{
    return (Rect.TopLeftX <= static_cast<float>(InPoint.X) && static_cast<float>(InPoint.X) <= Rect.TopLeftX + Rect.Width) &&
           (Rect.TopLeftY <= static_cast<float>(InPoint.Y) && static_cast<float>(InPoint.Y) <= Rect.TopLeftY + Rect.Height);
}

HRESULT FViewportResource::CreateCubeShadowMapResources() {
    HRESULT hr = S_OK;

    UINT TextureHeight = 1024;
    UINT TextureWidth = 1024;
    
    // 큐브맵 배열 텍스처 생성
    D3D11_TEXTURE2D_DESC cubeDesc = {};
    cubeDesc.Format = DXGI_FORMAT_R32G32_TYPELESS;
    cubeDesc.MipLevels = 4;
    cubeDesc.ArraySize = 6 * MAX_POINT_LIGHT; // 6면 × 라이트 수
    cubeDesc.Usage = D3D11_USAGE_DEFAULT;
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    cubeDesc.Height = TextureHeight;
    cubeDesc.Width = TextureWidth;
    cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&cubeDesc, nullptr, &PointShadowMapArrayTexture);
    if (FAILED(hr)) return hr;

    // 전체 큐브맵 배열 SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    srvDesc.TextureCubeArray.MipLevels = -1;
    srvDesc.TextureCubeArray.NumCubes = MAX_POINT_LIGHT;
    srvDesc.TextureCubeArray.MostDetailedMip = 0;

    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(
        PointShadowMapArrayTexture, &srvDesc, &PointShadowMapArraySRV);
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC ShadowMapTextureDepthDesc = {};
    ShadowMapTextureDepthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    ShadowMapTextureDepthDesc.MipLevels = 0;
    ShadowMapTextureDepthDesc.ArraySize = 6 * MAX_POINT_LIGHT;
    ShadowMapTextureDepthDesc.Usage = D3D11_USAGE_DEFAULT;
    ShadowMapTextureDepthDesc.SampleDesc.Count = 1;
    ShadowMapTextureDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ShadowMapTextureDepthDesc.Height = TextureHeight;
    ShadowMapTextureDepthDesc.Width = TextureWidth;

    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(
        &ShadowMapTextureDepthDesc, nullptr, &PointShadowMapArrayDepthTexture);
    if (FAILED(hr)) return hr;
    
    for (int32 i = 0; i < MAX_POINT_LIGHT; ++i) {
        for (int face = 0; face < 6; ++face) {
            D3D11_RENDER_TARGET_VIEW_DESC ShadowMapRenderTargetViewDesc = {};
            ShadowMapRenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            ShadowMapRenderTargetViewDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            ShadowMapRenderTargetViewDesc.Texture2DArray.MipSlice = 0;
            ShadowMapRenderTargetViewDesc.Texture2DArray.ArraySize = 1;
            ShadowMapRenderTargetViewDesc.Texture2DArray.FirstArraySlice = i * 6 + face;

            hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(
                PointShadowMapArrayTexture, &ShadowMapRenderTargetViewDesc, &PointShadowMapRTVs[i*6+face]
            );
            if (FAILED(hr)) return hr;
            
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.ArraySize = 1;
            dsvDesc.Texture2DArray.FirstArraySlice = i * 6 + face;

            hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(
                PointShadowMapArrayDepthTexture, &dsvDesc,
                &PointLightDSVs[i * 6 + face]); 
            if (FAILED(hr)) return hr;
        }
    }

    // 면별 SRV 생성
    hr = CreatePointShadowMapFaceSRVs();
    return hr;
}

HRESULT FViewportResource::CreatePointShadowMapFaceSRVs() {
    HRESULT hr = S_OK;

    D3D11_SHADER_RESOURCE_VIEW_DESC faceDesc = {};
    faceDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    faceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    faceDesc.Texture2DArray.MipLevels = -1;
    faceDesc.Texture2DArray.ArraySize = 1;
    
    for (int32 light = 0; light < MAX_POINT_LIGHT; ++light) {
        for (int32 face = 0; face < 6; ++face) {
            faceDesc.Texture2DArray.FirstArraySlice = light * 6 + face;
            hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(
                PointShadowMapArrayTexture,
                &faceDesc,
                &PointShadowMapFaceSRVs[light * 6 + face]
            );
            if (FAILED(hr)) return hr;
        }
    }
    return hr;
}

