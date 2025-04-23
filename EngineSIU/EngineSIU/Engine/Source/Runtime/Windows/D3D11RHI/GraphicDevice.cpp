#include "GraphicDevice.h"
#include <cwchar>
#include <Components/HeightFogComponent.h>
#include <UObject/UObjectIterator.h>
#include <Engine/Engine.h>
#include "PropertyEditor/ShowFlags.h"

void FGraphicsDevice::Initialize(HWND hWindow)
{
    CreateDeviceAndSwapChain(hWindow);
    CreateBackBuffer();
    CreateDepthStencilState();
    CreateRasterizerState();
    CreateAlphaBlendState();
    CurrentRasterizer = RasterizerSolidBack;
}

void FGraphicsDevice::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 스왑 체인 설정 구조체 초기화
    SwapchainDesc.BufferDesc.Width = 0;                           // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Height = 0;                          // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Format = BackBufferFormat; // 색상 포맷
    SwapchainDesc.SampleDesc.Count = 1;                           // 멀티 샘플링 비활성화
    SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 렌더 타겟으로 사용
    SwapchainDesc.BufferCount = 2;                                // 더블 버퍼링
    SwapchainDesc.OutputWindow = hWindow;                         // 렌더링할 창 핸들
    SwapchainDesc.Windowed = TRUE;                                // 창 모드
    SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;     // 스왑 방식

    // 디바이스와 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
        &SwapchainDesc, &SwapChain, &Device, nullptr, &DeviceContext
    );

    if (FAILED(hr))
    {
        MessageBox(hWindow, L"CreateDeviceAndSwapChain failed!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 스왑 체인 정보 가져오기 (이후에 사용을 위해)
    SwapChain->GetDesc(&SwapchainDesc);
    ScreenWidth = SwapchainDesc.BufferDesc.Width;
    ScreenHeight = SwapchainDesc.BufferDesc.Height;

    Viewport.Width = ScreenWidth;
    Viewport.Height = ScreenHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
}

ID3D11Texture2D* FGraphicsDevice::CreateTexture2D(const D3D11_TEXTURE2D_DESC& Description, const void* InitialData)
{
    D3D11_SUBRESOURCE_DATA Data = {};
    Data.pSysMem = InitialData;
    Data.SysMemPitch = Description.Width * 4;
    
    ID3D11Texture2D* Texture2D;
    HRESULT hr = Device->CreateTexture2D(&Description, InitialData ? &Data : nullptr, &Texture2D);
    
    if (FAILED(hr))
    {
        return nullptr;
    }
    
    return Texture2D;
}

void FGraphicsDevice::CreateDepthStencilState()
{
    // DepthStencil 상태 설명 설정
    D3D11_DEPTH_STENCIL_DESC DepthStencilStateDesc;

    // Depth test parameters
    DepthStencilStateDesc.DepthEnable = true;
    DepthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DepthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    DepthStencilStateDesc.StencilEnable = true;
    DepthStencilStateDesc.StencilReadMask = 0xFF;
    DepthStencilStateDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    DepthStencilStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    DepthStencilStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    DepthStencilStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    DepthStencilStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    DepthStencilStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    DepthStencilStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    DepthStencilStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    DepthStencilStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    //// DepthStencil 상태 생성
    HRESULT hr = Device->CreateDepthStencilState(&DepthStencilStateDesc, &DepthStencilState);
    if (FAILED(hr))
    {
        // 오류 처리
        return;
    }
}

void FGraphicsDevice::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC RasterizerDesc = {};

    RasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    RasterizerDesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&RasterizerDesc, &RasterizerWireframeBack);

    RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    RasterizerDesc.CullMode = D3D11_CULL_FRONT;
    Device->CreateRasterizerState(&RasterizerDesc, &RasterizerSolidFront);
    
    RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    RasterizerDesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&RasterizerDesc, &RasterizerSolidBack);

    RasterizerDesc.FrontCounterClockwise = FALSE;
    RasterizerDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
    RasterizerDesc.DepthBias = 3;
    RasterizerDesc.SlopeScaledDepthBias = 3.0f;
    RasterizerDesc.DepthClipEnable = TRUE;
    RasterizerDesc.ScissorEnable = FALSE;
    RasterizerDesc.MultisampleEnable = FALSE;
    RasterizerDesc.AntialiasedLineEnable = FALSE;
    Device->CreateRasterizerState(&RasterizerDesc, &RasterizerShadowMapBack);
}

void FGraphicsDevice::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

void FGraphicsDevice::CreateBackBuffer()
{
    SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBufferTexture));

    D3D11_RENDER_TARGET_VIEW_DESC BackBufferRTVDesc = {};
    BackBufferRTVDesc.Format = BackBufferRTVFormat;
    BackBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    HRESULT hr = Device->CreateRenderTargetView(BackBufferTexture, &BackBufferRTVDesc, &BackBufferRTV);
    if (FAILED(hr))
    {
        return;
    }
}

void FGraphicsDevice::ReleaseFrameBuffer()
{
    if (BackBufferRTV)
    {
        BackBufferRTV->Release();
        BackBufferRTV = nullptr;
    }

    if (BackBufferTexture)
    {
        BackBufferTexture->Release();
        BackBufferTexture = nullptr;
    }
}

void FGraphicsDevice::ReleaseRasterizerState()
{
    if (RasterizerSolidBack)
    {
        RasterizerSolidBack->Release();
        RasterizerSolidBack = nullptr;
    }
    if (RasterizerSolidFront)
    {
        RasterizerSolidFront->Release();
        RasterizerSolidFront = nullptr;
    }
    if (RasterizerWireframeBack)
    {
        RasterizerWireframeBack->Release();
        RasterizerWireframeBack = nullptr;
    }
}

void FGraphicsDevice::ReleaseDepthStencilResources()
{
    // 깊이/스텐실 상태 해제
    if (DepthStencilState)
    {
        DepthStencilState->Release();
        DepthStencilState = nullptr;
    }
}

void FGraphicsDevice::Release()
{
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseRasterizerState();
    ReleaseDepthStencilResources();
    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

void FGraphicsDevice::SwapBuffer() const
{
    SwapChain->Present(1, 0);
}

void FGraphicsDevice::Resize(HWND hWindow)
{
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();

    if (ScreenWidth == 0 || ScreenHeight == 0)
    {
        MessageBox(hWindow, L"Invalid width or height for ResizeBuffers!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // SwapChain 크기 조정
    HRESULT hr = S_OK;
    hr = SwapChain->ResizeBuffers(0, 0, 0, BackBufferFormat, 0); // DXGI_FORMAT_B8G8R8A8_UNORM으로 시도
    if (FAILED(hr))
    {
        MessageBox(hWindow, L"failed", L"ResizeBuffers failed ", MB_ICONERROR | MB_OK);
        return;
    }

    SwapChain->GetDesc(&SwapchainDesc);
    ScreenWidth = SwapchainDesc.BufferDesc.Width;
    ScreenHeight = SwapchainDesc.BufferDesc.Height;

    Viewport.Width = ScreenWidth;
    Viewport.Height = ScreenHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;

    CreateBackBuffer();
}

void FGraphicsDevice::CreateAlphaBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Device->CreateBlendState(&blendDesc, &AlphaBlendState);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"AlphaBlendState 생성에 실패했습니다!", L"Error", MB_ICONERROR | MB_OK);
    }
}

void FGraphicsDevice::ChangeRasterizer(EViewModeIndex ViewModeIndex)
{
    switch (ViewModeIndex)
    {
    case EViewModeIndex::VMI_Wireframe:
        CurrentRasterizer = RasterizerWireframeBack;
        break;
    case EViewModeIndex::VMI_Lit_Gouraud:
    case EViewModeIndex::VMI_Lit_Lambert:
    case EViewModeIndex::VMI_Lit_BlinnPhong:
    case EViewModeIndex::VMI_Unlit:
    case EViewModeIndex::VMI_SceneDepth:
    case EViewModeIndex::VMI_WorldNormal:
        CurrentRasterizer = RasterizerSolidBack;
        break;
    }
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정
}

void FGraphicsDevice::CreateRTV(ID3D11Texture2D*& OutTexture, ID3D11RenderTargetView*& OutRTV)
{
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = ScreenWidth;
    TextureDesc.Height = ScreenHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    Device->CreateTexture2D(&TextureDesc, nullptr, &OutTexture);

    D3D11_RENDER_TARGET_VIEW_DESC FogRTVDesc = {};
    FogRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    FogRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(OutTexture, &FogRTVDesc, &OutRTV);
}

void FGraphicsDevice::Prepare()
{
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    DeviceContext->ClearRenderTargetView(BackBufferRTV, ClearColor);
}

/* TODO: 픽셀 피킹 관련 함수로, 임시로 주석 처리
uint32 FGraphicsDevice::GetPixelUUID(POINT pt) const
{
    // pt.x 값 제한하기
    if (pt.x < 0)
    {
        pt.x = 0;
    }
    else if (pt.x > ScreenWidth)
    {
        pt.x = ScreenWidth;
    }

    // pt.y 값 제한하기
    if (pt.y < 0)
    {
        pt.y = 0;
    }
    else if (pt.y > ScreenHeight)
    {
        pt.y = ScreenHeight;
    }

    // 1. Staging 텍스처 생성 (1x1 픽셀)
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = 1; // 픽셀 1개만 복사
    stagingDesc.Height = 1;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 원본 텍스처 포맷과 동일
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);

    // 2. 복사할 영역 정의 (D3D11_BOX)
    D3D11_BOX srcBox;
    srcBox.left = static_cast<UINT>(pt.x);
    srcBox.right = srcBox.left + 1; // 1픽셀 너비
    srcBox.top = static_cast<UINT>(pt.y);
    srcBox.bottom = srcBox.top + 1; // 1픽셀 높이
    srcBox.front = 0;
    srcBox.back = 1;
    FVector4 UUIDColor{1, 1, 1, 1};

    if (stagingTexture == nullptr)
        return DecodeUUIDColor(UUIDColor);

    // 3. 특정 좌표만 복사
    DeviceContext->CopySubresourceRegion(
        stagingTexture,  // 대상 텍스처
        0,               // 대상 서브리소스
        0, 0, 0,         // 대상 좌표 (x, y, z)
        UUIDFrameBuffer, // 원본 텍스처
        0,               // 원본 서브리소스
        &srcBox          // 복사 영역
    );

    // 4. 데이터 매핑
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    DeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    // 5. 픽셀 데이터 추출 (1x1 텍스처이므로 offset = 0)
    const BYTE* pixelData = static_cast<const BYTE*>(mapped.pData);

    if (pixelData)
    {
        UUIDColor.X = static_cast<float>(pixelData[0]); // R
        UUIDColor.Y = static_cast<float>(pixelData[1]); // G
        UUIDColor.Z = static_cast<float>(pixelData[2]); // B
        UUIDColor.W = static_cast<float>(pixelData[3]); // A
    }

    // 6. 매핑 해제 및 정리
    DeviceContext->Unmap(stagingTexture, 0);
    if (stagingTexture) stagingTexture->Release();
    stagingTexture = nullptr;

    return DecodeUUIDColor(UUIDColor);
}

uint32 FGraphicsDevice::DecodeUUIDColor(FVector4 UUIDColor) const
{
    const uint32_t W = static_cast<uint32_t>(UUIDColor.W) << 24;
    const uint32_t Z = static_cast<uint32_t>(UUIDColor.Z) << 16;
    const uint32_t Y = static_cast<uint32_t>(UUIDColor.Y) << 8;
    const uint32_t X = static_cast<uint32_t>(UUIDColor.X);

    return W | Z | Y | X;
}
*/
