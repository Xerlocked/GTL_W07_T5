#include "Components/SceneComponent.h"
#include "Math/Rotator.h"
#include "Math/JungleMath.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

USceneComponent::USceneComponent()
    : RelativeLocation(FVector(0.f, 0.f, 0.f))
    , RelativeRotation(FVector(0.f, 0.f, 0.f))
    , RelativeScale3D(FVector(1.f, 1.f, 1.f))
{
}

UObject* USceneComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->RelativeLocation = RelativeLocation;
    NewComponent->RelativeRotation = RelativeRotation;
    NewComponent->RelativeScale3D = RelativeScale3D;

    return NewComponent;
}

void USceneComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("RelativeLocation"), *RelativeLocation.ToString());
    OutProperties.Add(TEXT("RelativeRotation"), *RelativeRotation.ToString());
    OutProperties.Add(TEXT("RelativeScale3D"), *RelativeScale3D.ToString());
    
}

void USceneComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("RelativeLocation"));
    if (TempStr)
    {
        RelativeLocation.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("RelativeRotation"));
    if (TempStr)
    {
        RelativeRotation.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("RelativeScale3D"));
    if (TempStr)
    {
        RelativeScale3D.InitFromString(*TempStr);
    }
}

void USceneComponent::InitializeComponent()
{
    Super::InitializeComponent();

}

void USceneComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
}


int USceneComponent::CheckRayIntersection(FVector& InRayOrigin, FVector& InRayDirection, float& pfNearHitDistance)
{
    // TODO: 나중에 지워도 될듯
    int nIntersections = 0;
    return nIntersections;
}

void USceneComponent::DestroyComponent()
{
    if (AttachParent)
    {
        AttachParent->AttachChildren.Remove(this);
        AttachParent = nullptr;
    }
    Super::DestroyComponent();
}

FVector USceneComponent::GetForwardVector()
{
    return GetWorldMatrix().GetAxis(0).GetSafeNormal();
}

FVector USceneComponent::GetRightVector()
{
	FVector Right = FVector(0.f, 1.f, 0.0f);
	Right = JungleMath::FVectorRotate(Right, RelativeRotation);
	return Right;
}

FVector USceneComponent::GetUpVector()
{
	FVector Up = FVector(0.f, 0.f, 1.0f);
	Up = JungleMath::FVectorRotate(Up, RelativeRotation);
	return Up;
}


void USceneComponent::AddLocation(FVector InAddValue)
{
	RelativeLocation = RelativeLocation + InAddValue;

}

void USceneComponent::AddRotation(FVector InAddValue)
{
	RelativeRotation = RelativeRotation + InAddValue;

}

void USceneComponent::AddScale(FVector InAddValue)
{
	RelativeScale3D = RelativeScale3D + InAddValue;

}

void USceneComponent::AttachToComponent(USceneComponent* InParent)
{
    // 기존 부모와 연결을 끊기
    if (AttachParent)
    {
        AttachParent->AttachChildren.Remove(this);
    }

    // InParent도 nullptr이면 부모를 nullptr로 설정
    if (InParent == nullptr)
    {
        AttachParent = nullptr;
        return;
    }


    // 새로운 부모 설정
    AttachParent = InParent;

    // 부모의 자식 리스트에 추가
    if (!InParent->AttachChildren.Contains(this))
    {
        InParent->AttachChildren.Add(this);
    }
}

FVector USceneComponent::GetWorldLocation() const
{
    if (AttachParent)
    {
        return AttachParent->GetWorldLocation() + RelativeLocation;
    }
    return RelativeLocation;
}

FRotator USceneComponent::GetWorldRotation() const
{
    if (AttachParent)
    {
        return AttachParent->GetWorldRotation().ToQuaternion() * RelativeRotation.ToQuaternion();
    }
    return RelativeRotation;
}

FVector USceneComponent::GetWorldScale3D() const
{
    if (AttachParent)
    {
        return AttachParent->GetWorldScale3D() * RelativeScale3D;
    }
    return RelativeScale3D;
}

FMatrix USceneComponent::GetScaleMatrix() const
{
    FMatrix ScaleMat = FMatrix::GetScaleMatrix(RelativeScale3D);
    if (AttachParent)
    {
        FMatrix ParentScaleMat = AttachParent->GetScaleMatrix();
        ScaleMat = ScaleMat * ParentScaleMat;
    }
    return ScaleMat;
}

FMatrix USceneComponent::GetRotationMatrix() const
{
    FMatrix RotationMat = FMatrix::GetRotationMatrix(RelativeRotation);
    if (AttachParent)
    {
        FMatrix ParentRotationMat = AttachParent->GetRotationMatrix();
        RotationMat = RotationMat * ParentRotationMat;
    }
    return RotationMat;
}

FMatrix USceneComponent::GetTranslationMatrix() const
{
    FMatrix TranslationMat = FMatrix::GetTranslationMatrix(RelativeLocation);
    if (AttachParent)
    {
        FMatrix ParentTranslationMat = AttachParent->GetTranslationMatrix();
        TranslationMat = TranslationMat * ParentTranslationMat;
    }
    return TranslationMat;
}

FMatrix USceneComponent::GetWorldMatrix() const
{
    FMatrix ScaleMat = FMatrix::GetScaleMatrix(RelativeScale3D);
    FMatrix RotationMat = FMatrix::GetRotationMatrix(RelativeRotation);
    FMatrix TranslationMat = FMatrix::GetTranslationMatrix(RelativeLocation);

    FMatrix RTMat = RotationMat * TranslationMat;
    if (AttachParent)
    {
        FMatrix ParentScaleMat = AttachParent->GetScaleMatrix();
        FMatrix ParentRotationMat = AttachParent->GetRotationMatrix();
        FMatrix ParentTranslationMat = AttachParent->GetTranslationMatrix();
        
        ScaleMat = ScaleMat * ParentScaleMat;
        FMatrix ParentRTMat = ParentRotationMat * ParentTranslationMat;
        RTMat = RTMat * ParentRTMat;
    }
    return ScaleMat * RTMat;
}

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
    if (
        InParent != AttachParent                                  // 설정하려는 Parent가 기존의 Parent와 다르거나
        && InParent != this                                       // InParent가 본인이 아니고
        && InParent != nullptr                                    // InParent가 유효한 포인터 이며
        && (
            AttachParent == nullptr                               // AttachParent도 유효하며
            || !AttachParent->AttachChildren.Contains(this)  // 한번이라도 SetupAttachment가 호출된적이 없는 경우
        ) 
    ) {
        AttachParent = InParent;

        // TODO: .AddUnique의 실행 위치를 RegisterComponent로 바꾸거나 해야할 듯
        InParent->AttachChildren.AddUnique(this);
    }
}
