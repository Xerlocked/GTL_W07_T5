#include "EditorViewportClient.h"
#include "fstream"
#include "sstream"
#include "ostream"
#include "Math/JungleMath.h"
#include "UnrealClient.h"
#include "WindowsCursor.h"
#include "World/World.h"
#include "GameFramework/Actor.h"
#include "Engine/EditorEngine.h"

#include "UObject/ObjectFactory.h"
#include "BaseGizmos/TransformGizmo.h"
#include "LevelEditor/SLevelEditor.h"
#include "SlateCore/Input/Events.h"

FVector FEditorViewportClient::Pivot = FVector(0.0f, 0.0f, 0.0f);
float FEditorViewportClient::OrthoSize = 10.0f;

FEditorViewportClient::FEditorViewportClient()
    : Viewport(nullptr)
    , ViewportType(LVT_Perspective)
    , ShowFlag(31)
    , ViewMode(EViewModeIndex::VMI_Lit_BlinnPhong)
{
}

FEditorViewportClient::~FEditorViewportClient()
{
    Release();

    ViewportResourceCache = nullptr;
}

void FEditorViewportClient::Draw(FViewport* Viewport)
{
}

void FEditorViewportClient::Initialize(EViewScreenLocation InViewportIndex, const FRect& InRect)
{
    ViewportIndex = static_cast<int32>(InViewportIndex);
    
    PerspectiveCamera.SetLocation(FVector(8.0f, 8.0f, 8.f));
    PerspectiveCamera.SetRotation(FVector(0.0f, 45.0f, -135.0f));
    
    Viewport = new FViewport(InViewportIndex);
    Viewport->Initialize(InRect);

    GizmoActor = FObjectFactory::ConstructObject<ATransformGizmo>(GEngine); // TODO : EditorEngine 외의 다른 Engine 형태가 추가되면 GEngine 대신 다른 방식으로 넣어주어야 함.
    GizmoActor->Initialize(this);
}

void FEditorViewportClient::Tick(float DeltaTime)
{
    UpdateEditorCameraMovement(DeltaTime);
    UpdateViewMatrix();
    UpdateProjectionMatrix();
    GizmoActor->Tick(DeltaTime);
}

void FEditorViewportClient::Release() const
{
    delete Viewport;
}

void FEditorViewportClient::UpdateEditorCameraMovement(float DeltaTime)
{
    if (PressedKeys.Contains(EKeys::A))
    {
        CameraMoveRight(-100.f * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::D))
    {
        CameraMoveRight(100.f * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::W))
    {
        CameraMoveForward(100.f * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::S))
    {
        CameraMoveForward(-100.f * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::E))
    {
        CameraMoveUp(100.f * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::Q))
    {
        CameraMoveUp(-100.f * DeltaTime);
    }
}

void FEditorViewportClient::InputKey(const FKeyEvent& InKeyEvent)
{
    // TODO: 나중에 InKeyEvent.GetKey();로 가져오는걸로 수정하기

    // 마우스 우클릭이 되었을때만 실행되는 함수
    if (GetKeyState(VK_RBUTTON) & 0x8000)
    {
        switch (InKeyEvent.GetCharacter())
        {
        case 'A':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::A);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::A);
            }
            break;
        }
        case 'D':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::D);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::D);
            }
            break;
        }
        case 'W':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::W);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::W);
            }
            break;
        }
        case 'S':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::S);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::S);
            }
            break;
        }
        case 'E':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::E);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::E);
            }
            break;
        }
        case 'Q':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::Q);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::Q);
            }
            break;
        }
        default:
            break;
        }
    }
    else
    {
        AEditorPlayer* EdPlayer = CastChecked<UEditorEngine>(GEngine)->GetEditorPlayer();
        switch (InKeyEvent.GetCharacter())
        {
        case 'W':
        {
            EdPlayer->SetMode(CM_TRANSLATION);
            break;
        }
        case 'E':
        {
            EdPlayer->SetMode(CM_ROTATION);
            break;
        }
        case 'R':
        {
            EdPlayer->SetMode(CM_SCALE);
            break;
        }
        default:
            break;
        }
        PressedKeys.Empty();
    }


    // 일반적인 단일 키 이벤트
    if (InKeyEvent.GetInputEvent() == IE_Pressed)
    {
        switch (InKeyEvent.GetCharacter())
        {
        case 'F':
        {
            const UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            if (const AActor* PickedActor = Engine->GetSelectedActor())
            {
                FViewportCamera& ViewTransform = PerspectiveCamera;
                ViewTransform.SetLocation(
                    // TODO: 10.0f 대신, 정점의 min, max의 거리를 구해서 하면 좋을 듯
                    PickedActor->GetActorLocation() - (ViewTransform.GetForwardVector() * 10.0f)
                );
            }
            break;
        }
        case 'M':
        {
            FEngineLoop::GraphicDevice.Resize(GEngineLoop.AppWnd);
            SLevelEditor* LevelEd = GEngineLoop.GetLevelEditor();
            LevelEd->SetEnableMultiViewport(!LevelEd->IsMultiViewport());
            break;
        }
        default:
            break;
        }

        // Virtual Key
        UEditorEngine* EdEngine = CastChecked<UEditorEngine>(GEngine);
        switch (InKeyEvent.GetKeyCode())
        {
        case VK_DELETE:
        {
            if (AActor* SelectedActor = EdEngine->GetSelectedActor())
            {
                EdEngine->DeselectActor(SelectedActor);
                GEngine->ActiveWorld->DestroyActor(SelectedActor);
            }
            break;
        }
        case VK_SPACE:
        {
            EdEngine->GetEditorPlayer()->AddControlMode();
            break;
        }
        default:
            break;
        }
    }
    return;
}

void FEditorViewportClient::MouseMove(const FPointerEvent& InMouseEvent)
{
    const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();

    // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
    if (IsPerspective()) {
        CameraRotateYaw(DeltaX * 0.1f);  // X 이동에 따라 좌우 회전
        CameraRotatePitch(DeltaY * 0.1f);  // Y 이동에 따라 상하 회전
    }
    else
    {
        PivotMoveRight(DeltaX);
        PivotMoveUp(DeltaY);
    }
}

void FEditorViewportClient::ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right)
{
    if (Viewport)
    {
        Viewport->ResizeViewport(Top, Bottom, Left, Right);
    }
    else
    {
        UE_LOG(LogLevel::Error, "Viewport is nullptr");
    }
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

bool FEditorViewportClient::IsSelected(const FVector2D& InPoint) const
{
    return GetViewport()->bIsHovered(InPoint);
}

void FEditorViewportClient::DeprojectFVector2D(const FVector2D& ScreenPos, FVector& OutWorldOrigin, FVector& OutWorldDir) const
{
    const float TopLeftX = Viewport->GetD3DViewport().TopLeftX;
    const float TopLeftY = Viewport->GetD3DViewport().TopLeftY;
    const float Width = Viewport->GetD3DViewport().Width;
    const float Height = Viewport->GetD3DViewport().Height;

    // 뷰포트가 유효한 위치에 있는지?
    assert(0.0f <= Width && 0.0f <= Height);

    // 뷰포트의 NDC 위치
    const FVector2D NDC_Pos = {
        ((ScreenPos.X - TopLeftX) / Width * 2.0f) - 1.0f,
        1.0f - ((ScreenPos.Y - TopLeftY) / Height * 2.0f)
    };

    FVector RayOrigin = {NDC_Pos.X, NDC_Pos.Y, 0.0f};
    FVector RayEnd = {NDC_Pos.X, NDC_Pos.Y, 1.0f};

    // 스크린 좌표계에서 월드 좌표계로 변환
    const FMatrix InvProjView = FMatrix::Inverse(Projection) * FMatrix::Inverse(View);
    RayOrigin = InvProjView.TransformPosition(RayOrigin);
    RayEnd = InvProjView.TransformPosition(RayEnd);

    OutWorldOrigin = RayOrigin;
    OutWorldDir = (RayEnd - RayOrigin).GetSafeNormal();
}


D3D11_VIEWPORT& FEditorViewportClient::GetD3DViewport() const
{
    return Viewport->GetD3DViewport();
}

FViewportResource* FEditorViewportClient::GetViewportResource()
{
    if (!ViewportResourceCache)
    {
        ViewportResourceCache = Viewport->GetViewportResource();
    }
    return ViewportResourceCache;
}

void FEditorViewportClient::CameraMoveForward(float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc = CurCameraLoc + PerspectiveCamera.GetForwardVector() * GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.X += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveRight(float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc = CurCameraLoc + PerspectiveCamera.GetRightVector() * GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.Y += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveUp(float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc.Z = CurCameraLoc.Z + GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.Z += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraRotateYaw(float InValue)
{
    FVector CurCameraRot = PerspectiveCamera.GetRotation();
    CurCameraRot.Z += InValue ;
    PerspectiveCamera.SetRotation(CurCameraRot);
}

void FEditorViewportClient::CameraRotatePitch(float InValue)
{
    FVector CurCameraRot = PerspectiveCamera.GetRotation();
    CurCameraRot.Y = FMath::Clamp(CurCameraRot.Y + InValue, -89.f, 89.f);
    PerspectiveCamera.SetRotation(CurCameraRot);
}

void FEditorViewportClient::CameraLookAt(const FVector& InDirection)
{
    // DirectionVector이 이미 단위 벡터라고 가정
    FVector Dir = InDirection.GetSafeNormal();

    float PitchAngle = FMath::RadiansToDegrees(
        FMath::Atan2(-Dir.Z, FMath::Sqrt(Dir.X * Dir.X + Dir.Y * Dir.Y))
    );
    float YawAngle = FMath::RadiansToDegrees(
        FMath::Atan2(Dir.Y, Dir.X)
    );
    const float RollAngle = 0.0f;
    
    FVector NewCameraRot = FVector(PerspectiveCamera.GetRotation().X, PitchAngle, YawAngle);
    PerspectiveCamera.SetRotation(NewCameraRot);
}

void FEditorViewportClient::PivotMoveRight(float InValue)
{
    Pivot = Pivot + OrthogonalCamera.GetRightVector() * InValue * -0.05f;
}

void FEditorViewportClient::PivotMoveUp(float InValue)
{
    Pivot = Pivot + OrthogonalCamera.GetUpVector() * InValue * 0.05f;
}

void FEditorViewportClient::UpdateViewMatrix()
{
    if (IsPerspective())
    {
        FVector eye = PerspectiveCamera.GetLocation();
        FVector forward = PerspectiveCamera.GetForwardVector();
        FVector target = eye + forward;
        
        View = JungleMath::CreateViewMatrix(eye,
            target,
            FVector{ 0.0f,0.0f, 1.0f }
        );
    }
    else 
    {
        UpdateOrthoCameraLoc();
        if (ViewportType == LVT_OrthoXY || ViewportType == LVT_OrthoNegativeXY)
        {
            View = JungleMath::CreateViewMatrix(OrthogonalCamera.GetLocation(),
                Pivot, FVector(0.0f, -1.0f, 0.0f)
            );
        }
        else
        {
            View = JungleMath::CreateViewMatrix(OrthogonalCamera.GetLocation(),
                Pivot, FVector(0.0f, 0.0f, 1.0f)
            );
        }
    }
}

void FEditorViewportClient::UpdateProjectionMatrix()
{
    AspectRatio = GetViewport()->GetD3DViewport().Width / GetViewport()->GetD3DViewport().Height;
    if (IsPerspective())
    {
        Projection = JungleMath::CreateProjectionMatrix(
            FMath::DegreesToRadians(ViewFOV),
            AspectRatio,
            NearClip,
            FarClip
        );
    }
    else
    {
        // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
        float OrthoWidth = OrthoSize * AspectRatio;
        float OrthoHeight = OrthoSize;

        // 오쏘그래픽 투영 행렬 생성 (nearPlane, farPlane 은 기존 값 사용)
        Projection = JungleMath::CreateOrthoProjectionMatrix(
            OrthoWidth,
            OrthoHeight,
            NearClip,
            FarClip
        );
    }
}

bool FEditorViewportClient::IsOrthographic() const
{
    return !IsPerspective();
}

bool FEditorViewportClient::IsPerspective() const
{
    return (GetViewportType() == LVT_Perspective);
}

FVector FEditorViewportClient::GetCameraLocation() const
{
    if (IsPerspective())
    {
        return PerspectiveCamera.GetLocation();
    }
    return OrthogonalCamera.GetLocation();
}

float FEditorViewportClient::GetCameraLearClip() const
{
    return NearClip;
}

float FEditorViewportClient::GetCameraFarClip() const
{
    return FarClip;
}

ELevelViewportType FEditorViewportClient::GetViewportType() const
{
    ELevelViewportType EffectiveViewportType = ViewportType;
    if (EffectiveViewportType == LVT_None)
    {
        EffectiveViewportType = LVT_Perspective;
    }
    return EffectiveViewportType;
}

void FEditorViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
    ViewportType = InViewportType;
}

void FEditorViewportClient::UpdateOrthoCameraLoc()
{
    switch (ViewportType)
    {
    case LVT_OrthoXY: // Top
        OrthogonalCamera.SetLocation(Pivot + FVector::UpVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 90.0f, -90.0f));
        break;
    case LVT_OrthoXZ: // Front
        OrthogonalCamera.SetLocation(Pivot + FVector::ForwardVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 180.0f));
        break;
    case LVT_OrthoYZ: // Left
        OrthogonalCamera.SetLocation(Pivot + FVector::RightVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 270.0f));
        break;
    case LVT_OrthoNegativeXY: // Bottom
        OrthogonalCamera.SetLocation(Pivot + FVector::UpVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, -90.0f, 90.0f));
        break;
    case LVT_OrthoNegativeXZ: // Back
        OrthogonalCamera.SetLocation(Pivot + FVector::ForwardVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 0.0f));
        break;
    case LVT_OrthoNegativeYZ: // Right
        OrthogonalCamera.SetLocation(Pivot + FVector::RightVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 90.0f));
        break;
    case LVT_None:
    case LVT_Perspective:
    case LVT_MAX:
    default:
        break;
    }
}

void FEditorViewportClient::SetOthoSize(float InValue)
{
    OrthoSize += InValue;
    OrthoSize = FMath::Max(OrthoSize, 0.1f);
}

void FEditorViewportClient::SetCameraLocation(const FVector& InLocation)
{
    PerspectiveCamera.SetLocation(InLocation);
}

void FEditorViewportClient::LoadConfig(const TMap<FString, FString>& config)
{
    FString ViewportNum = std::to_string(ViewportIndex);
    CameraSpeedSetting = GetValueFromConfig(config, "CameraSpeedSetting" + ViewportNum, 1);
    CameraSpeed = GetValueFromConfig(config, "CameraSpeedScalar" + ViewportNum, 1.0f);
    GridSize = GetValueFromConfig(config, "GridSize"+ ViewportNum, 10.0f);
    PerspectiveCamera.ViewLocation.X = GetValueFromConfig(config, "PerspectiveCameraLocX" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewLocation.Y = GetValueFromConfig(config, "PerspectiveCameraLocY" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewLocation.Z = GetValueFromConfig(config, "PerspectiveCameraLocZ" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.X = GetValueFromConfig(config, "PerspectiveCameraRotX" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.Y = GetValueFromConfig(config, "PerspectiveCameraRotY" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.Z = GetValueFromConfig(config, "PerspectiveCameraRotZ" + ViewportNum, 0.0f);
    ShowFlag = GetValueFromConfig(config, "ShowFlag" + ViewportNum, 31.0f);
    ViewMode = static_cast<EViewModeIndex>(GetValueFromConfig(config, "ViewMode" + ViewportNum, 0));
    ViewportType = static_cast<ELevelViewportType>(GetValueFromConfig(config, "ViewportType" + ViewportNum, 3));
}

void FEditorViewportClient::SaveConfig(TMap<FString, FString>& config) const
{
    FString ViewportNum = std::to_string(ViewportIndex);
    config["CameraSpeedSetting"+ ViewportNum] = std::to_string(CameraSpeedSetting);
    config["CameraSpeedScalar"+ ViewportNum] = std::to_string(CameraSpeed);
    config["GridSize"+ ViewportNum] = std::to_string(GridSize);
    config["PerspectiveCameraLocX" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().X);
    config["PerspectiveCameraLocY" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().Y);
    config["PerspectiveCameraLocZ" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().Z);
    config["PerspectiveCameraRotX" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().X);
    config["PerspectiveCameraRotY" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().Y);
    config["PerspectiveCameraRotZ" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().Z);
    config["ShowFlag"+ ViewportNum] = std::to_string(ShowFlag);
    config["ViewMode" + ViewportNum] = std::to_string(int32(ViewMode));
    config["ViewportType" + ViewportNum] = std::to_string(int32(ViewportType));
}

TMap<FString, FString> FEditorViewportClient::ReadIniFile(const FString& filePath) const
{
    TMap<FString, FString> config;
    std::ifstream file(*filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void FEditorViewportClient::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config) const
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

void FEditorViewportClient::SetCameraSpeed(float InValue)
{
    CameraSpeed = FMath::Clamp(InValue, 0.1f, 200.0f);
}

FVector FViewportCamera::GetForwardVector() const
{
    FVector Forward = FVector(1.f, 0.f, 0.0f);
    Forward = JungleMath::FVectorRotate(Forward, ViewRotation);
    return Forward;
}

FVector FViewportCamera::GetRightVector() const
{
    FVector Right = FVector(0.f, 1.f, 0.0f);
	Right = JungleMath::FVectorRotate(Right, ViewRotation);
	return Right;
}

FVector FViewportCamera::GetUpVector() const
{
    FVector Up = FVector(0.f, 0.f, 1.0f);
    Up = JungleMath::FVectorRotate(Up, ViewRotation);
    return Up;
}
