#include "EditorRenderPass.h"

#include "EngineLoop.h" // GEngineLoop
#include "Engine/Source/Runtime/Engine/Classes/Engine/Engine.h" // GEngine
#include "Engine/Source/Runtime/CoreUObject/UObject/Casts.h"
#include "Engine/Source/Runtime/Engine/Classes/Engine/EditorEngine.h"
#include <D3D11RHI/DXDShaderManager.h>

#include <d3dcompiler.h>

#include "UnrealClient.h"
#include "Engine/Source/Runtime/Engine/World/World.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Classes/Actors/Player.h"
#include "Engine/Classes/Components/Light/LightComponent.h"
#include "Engine/Classes/Components/Light/DirectionalLightComponent.h"
#include "Engine/Classes/Components/Light/SpotLightComponent.h"
#include "Engine/Classes/Components/Light/PointLightComponent.h"
#include "Engine/Classes/Components/HeightFogComponent.h"
#include "Engine/Classes/Components/Light/AmbientLightComponent.h"
#include "Engine/FLoaderOBJ.h"



void FEditorRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    
    CreateShaders();
    CreateBuffers();
    CreateConstantBuffers();
}

void FEditorRenderPass::Release()
{
    ReleaseShaders();
}

void FEditorRenderPass::CreateShaders()
{
    D3D11_INPUT_ELEMENT_DESC layoutGizmo[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    D3D11_INPUT_ELEMENT_DESC layoutPosOnly[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto AddShaderSet = [&](const std::wstring& keyPrefix, const std::string& vsEntry, const std::string& psEntry,
        D3D11_INPUT_ELEMENT_DESC* layout, uint32_t layoutSize,
        D3D11_PRIMITIVE_TOPOLOGY topology, FShaderResource& target)
        {
            ShaderManager->AddVertexShaderAndInputLayout(keyPrefix + L"VS", L"Shaders/EditorShader.hlsl", vsEntry, layout, layoutSize);
            ShaderManager->AddPixelShader(keyPrefix + L"PS", L"Shaders/EditorShader.hlsl", psEntry);
            target.Vertex = ShaderManager->GetVertexShaderByKey(keyPrefix + L"VS");
            target.Pixel = ShaderManager->GetPixelShaderByKey(keyPrefix + L"PS");
            target.Layout = ShaderManager->GetInputLayoutByKey(keyPrefix + L"VS");
            target.Topology = topology;
        };

    // Sphere
    AddShaderSet(L"Sphere", "sphereVS", "spherePS", layoutPosOnly, ARRAYSIZE(layoutPosOnly),
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST, Resources.Shaders.Sphere);

    // Cone
    AddShaderSet(L"Cone", "coneVS", "conePS", layoutPosOnly, ARRAYSIZE(layoutPosOnly),
        D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, Resources.Shaders.Cone);

    // Icons (layout 없음)
    AddShaderSet(L"Icon", "iconVS", "iconPS", layoutPosOnly, ARRAYSIZE(layoutPosOnly),
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Resources.Shaders.Icon);
    Resources.Shaders.Icon.Layout = nullptr;

    // Arrow (기즈모 layout 재사용)
    ShaderManager->AddVertexShaderAndInputLayout(L"ArrowVS", L"Shaders/EditorShader.hlsl", "arrowVS", layoutGizmo, ARRAYSIZE(layoutGizmo));
    ShaderManager->AddPixelShader(L"ArrowPS", L"Shaders/EditorShader.hlsl", "arrowPS");
    Resources.Shaders.Arrow.Vertex = ShaderManager->GetVertexShaderByKey(L"ArrowVS");
    Resources.Shaders.Arrow.Pixel = ShaderManager->GetPixelShaderByKey(L"ArrowPS");
    Resources.Shaders.Arrow.Layout = ShaderManager->GetInputLayoutByKey(L"ArrowVS");
    Resources.Shaders.Arrow.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void FEditorRenderPass::PrepareShader(FShaderResource ShaderResource) const
{
    Graphics->DeviceContext->VSSetShader(ShaderResource.Vertex, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(ShaderResource.Pixel, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(ShaderResource.Layout);
    Graphics->DeviceContext->IASetPrimitiveTopology(ShaderResource.Topology);
}

void FEditorRenderPass::ReleaseShaders()
{


}

void FEditorRenderPass::CreateBuffers()
{
    ////////////////////////////////////
    // Box 버퍼 생성
    TArray<FVector> CubeFrameVertices;
    CubeFrameVertices.Add({ -1.f, -1.f, -1.f }); // 0
    CubeFrameVertices.Add({ -1.f, 1.f, -1.f }); // 1
    CubeFrameVertices.Add({ 1.f, -1.f, -1.f }); // 2
    CubeFrameVertices.Add({ 1.f, 1.f, -1.f }); // 3
    CubeFrameVertices.Add({ -1.f, -1.f, 1.f }); // 4
    CubeFrameVertices.Add({ 1.f, -1.f, 1.f }); // 5
    CubeFrameVertices.Add({ -1.f, 1.f, 1.f }); // 6
    CubeFrameVertices.Add({ 1.f, 1.f, 1.f }); // 7

    TArray<uint32> CubeFrameIndices = {
        // Bottom face
        0, 1, 1, 3, 3, 2, 2, 0,
        // Top face
        4, 6, 6, 7, 7, 5, 5, 4,
        // Side faces
        0, 4, 1, 6, 2, 5, 3, 7
    };

    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    bufferDesc.ByteWidth = sizeof(FVector) * CubeFrameVertices.Num();
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = CubeFrameVertices.GetData();

    HRESULT hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Box.Vertex);
    if (FAILED(hr))
    {
        return;
    }

    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.ByteWidth = sizeof(uint32) * CubeFrameIndices.Num();

    initData.pSysMem = CubeFrameIndices.GetData();

    hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Box.Index);
    if (FAILED(hr))
    {
        return;
    }

    Resources.Primitives.Box.NumVertices = CubeFrameVertices.Num();
    Resources.Primitives.Box.VertexStride = sizeof(FVector);
    Resources.Primitives.Box.NumIndices = CubeFrameIndices.Num();

    ////////////////////////////////////
    // Sphere 버퍼 생성
    FVector SphereFrameVertices[] =
    {
        {1.0, 0.0, 0},
        {0.9795299412524945, 0.20129852008866006, 0},
        {0.9189578116202306, 0.39435585511331855, 0},
        {0.8207634412072763, 0.5712682150947923, 0},
        {0.6889669190756866, 0.72479278722912, 0},
        {0.5289640103269624, 0.8486442574947509, 0},
        {0.3473052528448203, 0.9377521321470804, 0},
        {0.1514277775045767, 0.9884683243281114, 0},
        {-0.05064916883871264, 0.9987165071710528, 0},
        {-0.2506525322587204, 0.9680771188662043, 0},
        {-0.4403941515576344, 0.8978045395707416, 0},
        {-0.6121059825476629, 0.7907757369376985, 0},
        {-0.758758122692791, 0.6513724827222223, 0},
        {-0.8743466161445821, 0.48530196253108104, 0},
        {-0.9541392564000488, 0.29936312297335804, 0},
        {-0.9948693233918952, 0.10116832198743228, 0},
        {-0.9948693233918952, -0.10116832198743204, 0},
        {-0.9541392564000489, -0.29936312297335776, 0},
        {-0.8743466161445822, -0.4853019625310808, 0},
        {-0.7587581226927911, -0.651372482722222, 0},
        {-0.6121059825476627, -0.7907757369376986, 0},
        {-0.44039415155763423, -0.8978045395707417, 0},
        {-0.2506525322587205, -0.9680771188662043, 0},
        {-0.05064916883871266, -0.9987165071710528, 0},
        {0.15142777750457667, -0.9884683243281114, 0},
        {0.3473052528448203, -0.9377521321470804, 0},
        {0.5289640103269624, -0.8486442574947509, 0},
        {0.6889669190756865, -0.72479278722912, 0},
        {0.8207634412072763, -0.5712682150947924, 0},
        {0.9189578116202306, -0.3943558551133187, 0},
        {0.9795299412524945, -0.20129852008866028, 0},
        {1, 0, 0},
        {1.0, 0, 0.0},
        {0.9795299412524945, 0, 0.20129852008866006},
        {0.9189578116202306, 0, 0.39435585511331855},
        {0.8207634412072763, 0, 0.5712682150947923},
        {0.6889669190756866, 0, 0.72479278722912},
        {0.5289640103269624, 0, 0.8486442574947509},
        {0.3473052528448203, 0, 0.9377521321470804},
        {0.1514277775045767, 0, 0.9884683243281114},
        {-0.05064916883871264, 0, 0.9987165071710528},
        {-0.2506525322587204, 0, 0.9680771188662043},
        {-0.4403941515576344, 0, 0.8978045395707416},
        {-0.6121059825476629, 0, 0.7907757369376985},
        {-0.758758122692791, 0, 0.6513724827222223},
        {-0.8743466161445821, 0, 0.48530196253108104},
        {-0.9541392564000488, 0, 0.29936312297335804},
        {-0.9948693233918952, 0, 0.10116832198743228},
        {-0.9948693233918952, 0, -0.10116832198743204},
        {-0.9541392564000489, 0, -0.29936312297335776},
        {-0.8743466161445822, 0, -0.4853019625310808},
        {-0.7587581226927911, 0, -0.651372482722222},
        {-0.6121059825476627, 0, -0.7907757369376986},
        {-0.44039415155763423, 0, -0.8978045395707417},
        {-0.2506525322587205, 0, -0.9680771188662043},
        {-0.05064916883871266, 0, -0.9987165071710528},
        {0.15142777750457667, 0, -0.9884683243281114},
        {0.3473052528448203, 0, -0.9377521321470804},
        {0.5289640103269624, 0, -0.8486442574947509},
        {0.6889669190756865, 0, -0.72479278722912},
        {0.8207634412072763, 0, -0.5712682150947924},
        {0.9189578116202306, 0, -0.3943558551133187},
        {0.9795299412524945, 0, -0.20129852008866028},
        {1, 0, 0},
        {0, 1.0, 0.0},
        {0, 0.9795299412524945, 0.20129852008866006},
        {0, 0.9189578116202306, 0.39435585511331855},
        {0, 0.8207634412072763, 0.5712682150947923},
        {0, 0.6889669190756866, 0.72479278722912},
        {0, 0.5289640103269624, 0.8486442574947509},
        {0, 0.3473052528448203, 0.9377521321470804},
        {0, 0.1514277775045767, 0.9884683243281114},
        {0, -0.05064916883871264, 0.9987165071710528},
        {0, -0.2506525322587204, 0.9680771188662043},
        {0, -0.4403941515576344, 0.8978045395707416},
        {0, -0.6121059825476629, 0.7907757369376985},
        {0, -0.758758122692791, 0.6513724827222223},
        {0, -0.8743466161445821, 0.48530196253108104},
        {0, -0.9541392564000488, 0.29936312297335804},
        {0, -0.9948693233918952, 0.10116832198743228},
        {0, -0.9948693233918952, -0.10116832198743204},
        {0, -0.9541392564000489, -0.29936312297335776},
        {0, -0.8743466161445822, -0.4853019625310808},
        {0, -0.7587581226927911, -0.651372482722222},
        {0, -0.6121059825476627, -0.7907757369376986},
        {0, -0.44039415155763423, -0.8978045395707417},
        {0, -0.2506525322587205, -0.9680771188662043},
        {0, -0.05064916883871266, -0.9987165071710528},
        {0, 0.15142777750457667, -0.9884683243281114},
        {0, 0.3473052528448203, -0.9377521321470804},
        {0, 0.5289640103269624, -0.8486442574947509},
        {0, 0.6889669190756865, -0.72479278722912},
        {0, 0.8207634412072763, -0.5712682150947924},
        {0, 0.9189578116202306, -0.3943558551133187},
        {0, 0.9795299412524945, -0.20129852008866028},
        {0, 1, 0}
    };

    uint32 SphereFrameIndices[] =
    {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
        11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20,
        21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30,
        31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
        41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50,
        51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 60,
        61, 61, 62, 62, 63, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70,
        71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80,
        81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90,
        91, 91, 92, 92, 93, 93, 94, 94, 95
    };

    // 버텍스 버퍼 생성
    bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    bufferDesc.ByteWidth = sizeof(SphereFrameVertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    initData = {};
    initData.pSysMem = SphereFrameVertices;

    hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Sphere.Vertex);
    if (FAILED(hr))
    {
        return;
    }

    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.ByteWidth = sizeof(SphereFrameIndices);

    initData.pSysMem = SphereFrameIndices;

    hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Sphere.Index);
    if (FAILED(hr))
    {
        return;
    }

    Resources.Primitives.Sphere.NumVertices = ARRAYSIZE(SphereFrameVertices);
    Resources.Primitives.Sphere.VertexStride = sizeof(FVector);
    Resources.Primitives.Sphere.NumIndices = ARRAYSIZE(SphereFrameIndices);



    ////////////////////////////////////
    // Cone 버퍼 생성
    // 0,0,0이 Apex
    // z=1이고, xy에서 r=1인 원이 밑변
    TArray<FVector> ConeVertices;
    ConeVertices.Add({0.0f, 0.0f, 0.0f}); // Apex
    for (int i = 0; i < NumSegments; i++)
    {
        // hlsl 내부에서 계산
        //float angle = 2.0f * 3.1415926535897932f * i / (float)NuzmSegments;
        //float x = cos(angle);
        //float y = sin(angle);
        float x = 0;
        float y = 0;
        ConeVertices.Add({x, y, 1.0f}); // Bottom
    }
    TArray<uint32> ConeIndices;
    uint32 vertexOffset0 = 1;
    // apex -> 밑면으로 가는 line
    for (int i = 0; i < NumSegments; i++)
    {
        ConeIndices.Add(0);
        ConeIndices.Add(vertexOffset0 + i);
    }
    // 밑변
    for (int i = 0; i < NumSegments; i++)
    {
        ConeIndices.Add(vertexOffset0 + i);
        ConeIndices.Add(vertexOffset0 + (i + 1) % NumSegments);
    }

    // cone을 덮는 sphere
    // xz plane
    //float deltaAngle = 2.0f * 3.1415926535897932f / (float)NumSegments;
    //float offsetAngle = deltaAngle * NumSegments / 8; // 45도 부터 시작
    for (int i = 0; i < NumSegments + 1; i++)
    {
        //float angle = 2.0f * 3.1415926535897932f * i / (float)NumSegments + offsetAngle;
        //float x = cos(angle) * sqrt(2.f);
        //float z = sin(angle) * sqrt(2.f);
        float x = 0;
        float z = 0;
        ConeVertices.Add({ x, 0, z });
    }
    uint32 vertexOffset1 = NumSegments + vertexOffset0;
    for (int i = 0; i < NumSegments; i++)
    {
        ConeIndices.Add(vertexOffset1 + i);
        ConeIndices.Add(vertexOffset1 + (i + 1));
    }
    // yz plane
    for (int i = 0; i < NumSegments + 1; i++)
    {
        //float angle = 2.0f * 3.1415926535897932f * i / (float)NumSegments + offsetAngle;
        //float y = cos(angle) * sqrt(2.f);
        //float z = sin(angle) * sqrt(2.f);
        float y = 0;
        float z = 0;
        ConeVertices.Add({ 0, y, z });
    }
    uint32 vertexOffset2 = NumSegments + 1 + vertexOffset1;
    for (int i = 0; i < NumSegments; i++)
    {
        ConeIndices.Add(vertexOffset2 + i);
        ConeIndices.Add(vertexOffset2 + (i + 1));
    }

    // 버텍스 버퍼 생성
    bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    bufferDesc.ByteWidth = ConeVertices.Num() * sizeof(FVector);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    initData = {};
    initData.pSysMem = ConeVertices.GetData();

    hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Cone.Vertex);
    if (FAILED(hr))
    {
        return;
    }

    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.ByteWidth = ConeIndices.Num() * sizeof(FVector);

    initData.pSysMem = ConeIndices.GetData();

    hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, &Resources.Primitives.Cone.Index);
    if (FAILED(hr))
    {
        return;
    }

    Resources.Primitives.Cone.NumVertices = ConeVertices.Num();
    Resources.Primitives.Cone.VertexStride = sizeof(FVector);
    Resources.Primitives.Cone.NumIndices = ConeIndices.Num();

}
void FEditorRenderPass::CreateConstantBuffers()
{
    auto CreateCB = [this](UINT size, ID3D11Buffer** outBuffer)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = size;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Graphics->Device->CreateBuffer(&desc, nullptr, outBuffer);
    };

    CreateCB(sizeof(FConstantBufferCamera), &Resources.ConstantBuffers.Camera00);
    CreateCB(sizeof(FConstantBufferDebugAABB) * ConstantBufferSizeAABB, &Resources.ConstantBuffers.AABB11);
    CreateCB(sizeof(FConstantBufferDebugSphere) * ConstantBufferSizeSphere, &Resources.ConstantBuffers.Sphere11);
    CreateCB(sizeof(FConstantBufferDebugCone) * ConstantBufferSizeCone, &Resources.ConstantBuffers.Cone11);
    CreateCB(sizeof(FConstantBufferDebugGrid), &Resources.ConstantBuffers.Grid11);
    CreateCB(sizeof(FConstantBufferDebugIcon), &Resources.ConstantBuffers.Icon11);
    CreateCB(sizeof(FConstantBufferDebugArrow), &Resources.ConstantBuffers.Arrow11);
}

void FEditorRenderPass::PrepareRendertarget(std::shared_ptr<FEditorViewportClient> Viewport)
{
    const EResourceType ResourceType = EResourceType::ERT_Editor;

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, ViewportResource->GetDepthStencilView());

}

void FEditorRenderPass::PrepareRender()
{
    ClearRenderArr();
    // gizmo 제외하고 넣기

    if (GEngine->ActiveWorld->WorldType != EWorldType::Editor)
    {
        return;
    }
    for (const auto& actor : TObjectRange<AActor>())
    {
        for (const auto& comp : actor->GetComponents())
        {
            // AABB용 static mesh component
            if (UStaticMeshComponent* staticmesh = Cast<UStaticMeshComponent>(comp))
            {
                if (!staticmesh->IsA<UGizmoBaseComponent>())
                {
                    Resources.Components.StaticMesh.Add(staticmesh);
                }
            }

            // light
            if (ULightComponentBase* light = Cast<ULightComponentBase>(comp))
            {
                Resources.Components.Light.Add(light);
            }

            // fog
            if (UHeightFogComponent* fog = Cast<UHeightFogComponent>(comp))
            {
                Resources.Components.Fog.Add(fog);
            }
        }
    }
}

void FEditorRenderPass::ClearRenderArr()
{
    Resources.Components.StaticMesh.Empty();
    Resources.Components.Light.Empty();
    Resources.Components.Fog.Empty();
}

void FEditorRenderPass::PrepareConstantbufferGlobal()
{
    if (Resources.ConstantBuffers.Camera00)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &Resources.ConstantBuffers.Camera00);
    }
}

void FEditorRenderPass::UpdateConstantbufferGlobal(FConstantBufferCamera Buffer)
{
    if (Resources.ConstantBuffers.Camera00)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.Camera00, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, &Buffer, sizeof(Buffer));
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.Camera00, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FEditorRenderPass::Render(std::shared_ptr<FEditorViewportClient> Viewport)
{
    static bool isLoaded = false;
    if (!isLoaded)
    {
        LazyLoad();
        isLoaded = true;
    }

    PrepareRendertarget(Viewport);
    
    // PrepareConstantbufferGlobal();

    // FConstantBufferCamera buf;
    // buf.ViewMatrix = Viewport->GetViewMatrix();
    // buf.ProjMatrix = Viewport->GetProjectionMatrix();
    // buf.CameraPos = Viewport->GetCameraLocation();
    // buf.CameraLookAt = ActiveViewport->ViewTransformPerspective.GetLookAt();
    // UpdateConstantbufferGlobal(buf);

    // ID3D11DepthStencilState* DepthStateEnable = Graphics->DepthStencilState;
    // Graphics->DeviceContext->OMSetDepthStencilState(DepthStateEnable, 0);

    RenderPointlightInstanced();
    RenderSpotlightInstanced();
    RenderArrows();    // Directional Light Arrow : Depth Test Enabled
    //RenderIcons(World, ActiveViewport); // 기존 렌더패스에서 아이콘 렌더하고 있으므로 제거

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11Buffer* NullBuffer[1] = { nullptr };
    Graphics->DeviceContext->VSSetConstantBuffers(11, 1, NullBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(11, 1, NullBuffer);
}

void FEditorRenderPass::PrepareShaderGizmo()
{

}

void FEditorRenderPass::PrepareConstantbufferGizmo()
{
}

void FEditorRenderPass::RenderAxis()
{
    PrepareShader(Resources.Shaders.AxisLine);

    Graphics->DeviceContext->Draw(6, 0);
}

void FEditorRenderPass::PrepareConstantbufferAABB()
{
    /*if (Resources.ConstantBuffers.AABB13)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(11, 1, &Resources.ConstantBuffers.AABB13);
    }*/
}

void FEditorRenderPass::UdpateConstantbufferAABBInstanced(TArray<FConstantBufferDebugAABB> Buffer)
{
    if (Buffer.Num() > ConstantBufferSizeAABB)
    {
        // 최대개수 초과
        // 코드 잘못짠거 아니면 오면안됨
        UE_LOG(LogLevel::Error, "Invalid Buffer Num");
        return;
    }
    if (Resources.ConstantBuffers.AABB11)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.AABB11, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, Buffer.GetData(), sizeof(FConstantBufferDebugAABB) * Buffer.Num()); // TArray이니까 실제 값을 받아와야함
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.AABB11, 0);
    }
}

void FEditorRenderPass::RenderPointlightInstanced()
{
    SetShaderAndPrepare(L"SphereVS", L"SpherePS", Resources.Shaders.Sphere);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &Resources.Primitives.Sphere.Vertex, &Resources.Primitives.Sphere.VertexStride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(Resources.Primitives.Sphere.Index, DXGI_FORMAT_R32_UINT, 0);

    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugSphere> BufferAll;
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComp))
        {
            FConstantBufferDebugSphere b;
            b.Position = PointLightComp->GetWorldLocation();
            b.Radius = PointLightComp->GetAttenuationRadius();
            BufferAll.Add(b);
        }

    }

    PrepareConstantbufferPointlight();
    int BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeSphere) * ConstantBufferSizeSphere; ++i)
    {
        TArray<FConstantBufferDebugSphere> SubBuffer;
        for (int j = 0; j < ConstantBufferSizeAABB; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            UdpateConstantbufferPointlightInstanced(SubBuffer);
            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Sphere.NumIndices, SubBuffer.Num(), 0, 0, 0);
        }
    }
}

void FEditorRenderPass::PrepareConstantbufferPointlight()
{
    if (Resources.ConstantBuffers.Sphere11)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(11, 1, &Resources.ConstantBuffers.Sphere11);
    }
}

void FEditorRenderPass::UdpateConstantbufferPointlightInstanced(TArray<FConstantBufferDebugSphere> Buffer)
{
    if (Buffer.Num() > ConstantBufferSizeSphere)
    {
        // 최대개수 초과
        // 코드 잘못짠거 아니면 오면안됨
        UE_LOG(LogLevel::Error, "Invalid Buffer Num");
        return;
    }
    if (Resources.ConstantBuffers.Sphere11)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.Sphere11, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, Buffer.GetData(), sizeof(FConstantBufferDebugSphere) * Buffer.Num()); // TArray이니까 실제 값을 받아와야함
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.Sphere11, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FEditorRenderPass::RenderSpotlightInstanced()
{
    SetShaderAndPrepare(L"ConeVS", L"ConePS", Resources.Shaders.Cone);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &Resources.Primitives.Cone.Vertex, &Resources.Primitives.Cone.VertexStride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(Resources.Primitives.Cone.Index, DXGI_FORMAT_R32_UINT, 0);

    constexpr FLinearColor OuterColor = FLinearColor(149.f/255.f, 198.f/255.f, 255.f/255.f, 255.f/255.f);
    constexpr FLinearColor InnerColor = FLinearColor(0.777f, 1.0f, 1.0f, 1.0f);
    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugCone> BufferAll;
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        if (USpotLightComponent* SpotComp = Cast<USpotLightComponent>(LightComp))
        {
            FConstantBufferDebugCone b;
            b.ApexPosition = SpotComp->GetWorldLocation();
            b.Radius = SpotComp->GetAttenuationRadius();
            b.Direction = SpotComp->GetDirection();
            b.Angle = FMath::DegreesToRadians(SpotComp->GetInnerDegree() / 2);
            b.Color = InnerColor;
            BufferAll.Add(b);
            
            b.Angle = FMath::DegreesToRadians(SpotComp->GetOuterDegree() / 2);
            b.Color = OuterColor;
            BufferAll.Add(b);
        }
    }

    PrepareConstantbufferSpotlight();
    int BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeCone) * ConstantBufferSizeCone; ++i)
    {
        TArray<FConstantBufferDebugCone> SubBuffer;
        for (int j = 0; j < ConstantBufferSizeCone; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            UdpateConstantbufferSpotlightInstanced(SubBuffer);

            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Cone.NumIndices, 2, 0, 0, 0);
        }
    }
}

void FEditorRenderPass::PrepareConstantbufferSpotlight()
{
    if (Resources.ConstantBuffers.Cone11)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(11, 1, &Resources.ConstantBuffers.Cone11);
    }
}

void FEditorRenderPass::UdpateConstantbufferSpotlightInstanced(TArray<FConstantBufferDebugCone> Buffer)
{
    if (Buffer.Num() > ConstantBufferSizeCone)
    {
        // 최대개수 초과
        // 코드 잘못짠거 아니면 오면안됨
        UE_LOG(LogLevel::Error, "Invalid Buffer Num");
        return;
    }
    if (Resources.ConstantBuffers.Cone11)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.Cone11, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, Buffer.GetData(), sizeof(FConstantBufferDebugCone) * Buffer.Num()); // TArray이니까 실제 값을 받아와야함
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.Cone11, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}


// 꼼수로 이미 로드된 리소스를 사용
// GUObjectArray에 안올라가게 우회
void FEditorRenderPass::LazyLoad()
{
    // Resourcemanager에서 로드된 texture의 포인터를 가져옴
    // FResourceMgr::Initialize에 추가되어야함
    Resources.IconTextures[IconType::DirectionalLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/DirectionalLight_64x.png");
    Resources.IconTextures[IconType::PointLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/PointLight_64x.png");
	Resources.IconTextures[IconType::SpotLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/SpotLight_64x.png");
    Resources.IconTextures[IconType::AmbientLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/AmbientLight_64x.png");
	Resources.IconTextures[IconType::ExponentialFog] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/ExponentialHeightFog_64.png");
	Resources.IconTextures[IconType::AtmosphericFog] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/AtmosphericFog_64.png");

    // Gizmo arrow 로드
    UStaticMesh* Mesh = FManagerOBJ::GetStaticMesh(L"Assets/GizmoTranslationZ.obj");
    Resources.Primitives.Arrow.Vertex = Mesh->GetRenderData()->VertexBuffer;
    Resources.Primitives.Arrow.Index = Mesh->GetRenderData()->IndexBuffer;
    Resources.Primitives.Arrow.NumVertices = Mesh->GetRenderData()->Vertices.Num();
    Resources.Primitives.Arrow.NumIndices = Mesh->GetRenderData()->Indices.Num();
    Resources.Primitives.Arrow.VertexStride = sizeof(FStaticMeshVertex); // Directional Light의 Arrow에 해당됨

}

void FEditorRenderPass::RenderIcons(const UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    // ULightComponentBase::CheckRayIntersection에서도 수정 필요
    const float IconScale = 0.3;
    PrepareShader(Resources.Shaders.Icon);
    UINT offset = 0;
    // input vertex index 없음

    PrepareConstantbufferIcon();
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        FConstantBufferDebugIcon b;
        b.Position = LightComp->GetWorldLocation();
        b.Scale = IconScale;
        UdpateConstantbufferIcon(b);

        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComp))
        {
            UpdateTextureIcon(IconType::PointLight);
        }
        else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComp))
        {
            UpdateTextureIcon(IconType::SpotLight);
        }
        else if (UDirectionalLightComponent* DirectionalLightComp = Cast<UDirectionalLightComponent>(LightComp))
        {
            UpdateTextureIcon(IconType::DirectionalLight);
        }
        else if (UAmbientLightComponent* AmbientLightComp = Cast<UAmbientLightComponent>(LightComp))
        {
            UpdateTextureIcon(IconType::AmbientLight);
        }
        else
        {
            // 잘못된 light 종류
            continue;
        };
        Graphics->DeviceContext->Draw(6, 0); // 내부에서 버텍스 사용중
    }

    for (UHeightFogComponent* FogComp : Resources.Components.Fog)
    {
        FConstantBufferDebugIcon b;
        b.Position = FogComp->GetWorldLocation();
        b.Scale = IconScale;
        UdpateConstantbufferIcon(b);
        UpdateTextureIcon(IconType::ExponentialFog);

        Graphics->DeviceContext->Draw(6, 0); // 내부에서 버텍스 사용중
    }
}

void FEditorRenderPass::PrepareConstantbufferIcon()
{
    if (Resources.ConstantBuffers.Icon11)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(11, 1, &Resources.ConstantBuffers.Icon11);
    }
}

void FEditorRenderPass::UdpateConstantbufferIcon(FConstantBufferDebugIcon Buffer)
{
    if (Resources.ConstantBuffers.Icon11)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.Icon11, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, &Buffer, sizeof(FConstantBufferDebugIcon)); // TArray이니까 실제 값을 받아와야함
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.Icon11, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FEditorRenderPass::UpdateTextureIcon(IconType type)
{
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &Resources.IconTextures[type]->TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Resources.IconTextures[type]->SamplerState);
}

void FEditorRenderPass::RenderArrows()
{
    // XYZ한번. Z는 중복으로 적용
    const float ArrowScale = 1;

    SetShaderAndPrepare(L"ArrowVS", L"ArrowPS", Resources.Shaders.Arrow);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &Resources.Primitives.Arrow.Vertex, &Resources.Primitives.Arrow.VertexStride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(Resources.Primitives.Arrow.Index, DXGI_FORMAT_R32_UINT, 0);

    PrepareConstantbufferArrow();
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        if (UDirectionalLightComponent* DLightComp = Cast<UDirectionalLightComponent>(LightComp))
        {
            FConstantBufferDebugArrow buf;
            buf.Position = DLightComp->GetWorldLocation();
            buf.ArrowScaleXYZ = ArrowScale;
            buf.Direction = DLightComp->GetDirection();
            buf.ArrowScaleZ = ArrowScale;
            UdpateConstantbufferArrow(buf);
            Graphics->DeviceContext->DrawIndexed(Resources.Primitives.Arrow.NumIndices, 0, 0);
        }
        if (USpotLightComponent* SpotComp = Cast<USpotLightComponent>(LightComp))
        {
            FConstantBufferDebugArrow buf;
            buf.Position = SpotComp->GetWorldLocation();
            buf.ArrowScaleXYZ = ArrowScale;
            buf.Direction = SpotComp->GetDirection();
            buf.ArrowScaleZ = ArrowScale;
            UdpateConstantbufferArrow(buf);
            Graphics->DeviceContext->DrawIndexed(Resources.Primitives.Arrow.NumIndices, 0, 0);
        }
    }
}

void FEditorRenderPass::PrepareConstantbufferArrow()
{
    if (Resources.ConstantBuffers.Arrow11)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(11, 1, &Resources.ConstantBuffers.Arrow11);
    }
}

void FEditorRenderPass::UdpateConstantbufferArrow(FConstantBufferDebugArrow Buffer)
{
    if (Resources.ConstantBuffers.Arrow11)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(Resources.ConstantBuffers.Arrow11, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, &Buffer, sizeof(FConstantBufferDebugArrow)); // TArray이니까 실제 값을 받아와야함
        Graphics->DeviceContext->Unmap(Resources.ConstantBuffers.Arrow11, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FEditorRenderPass::SetShaderAndPrepare(const std::wstring& VertexKey, const std::wstring& PixelKey, FShaderResource& ShaderSlot)
{
    ShaderSlot.Vertex = ShaderManager->GetVertexShaderByKey(VertexKey);
    ShaderSlot.Pixel = ShaderManager->GetPixelShaderByKey(PixelKey);
    ShaderSlot.Layout = ShaderManager->GetInputLayoutByKey(VertexKey);
    PrepareShader(ShaderSlot);
}
