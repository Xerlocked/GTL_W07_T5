#include "SpotLightComponent.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "UObject/Casts.h"

USpotLightComponent::USpotLightComponent()
{
    SpotLightInfo.Position = GetWorldLocation();
    SpotLightInfo.Radius = 30.0f;
    SpotLightInfo.Direction = GetForwardVector();
    SpotLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    SpotLightInfo.Intensity = 1000.0f;
    SpotLightInfo.Type = ELightType::SPOT_LIGHT;
    SpotLightInfo.InnerRad = 0.2618;
    SpotLightInfo.OuterRad = 0.5236;
    SpotLightInfo.Attenuation = 20.0f;
}

USpotLightComponent::~USpotLightComponent()
{
}
UObject* USpotLightComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->SpotLightInfo = SpotLightInfo;
    }

    return NewComponent;
}

void USpotLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("Position"), FString::Printf(TEXT("%s"), *SpotLightInfo.Position.ToString()));
    OutProperties.Add(TEXT("Radius"), FString::Printf(TEXT("%f"), SpotLightInfo.Radius));
    OutProperties.Add(TEXT("Direction"), FString::Printf(TEXT("%s"), *SpotLightInfo.Direction.ToString()));
    OutProperties.Add(TEXT("LightColor"), FString::Printf(TEXT("%s"), *SpotLightInfo.LightColor.ToString()));
    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), SpotLightInfo.Intensity));
    OutProperties.Add(TEXT("Type"), FString::Printf(TEXT("%d"), SpotLightInfo.Type));
    OutProperties.Add(TEXT("InnerRad"), FString::Printf(TEXT("%f"), SpotLightInfo.InnerRad));
    OutProperties.Add(TEXT("OuterRad"), FString::Printf(TEXT("%f"), SpotLightInfo.OuterRad));
    OutProperties.Add(TEXT("Attenuation"), FString::Printf(TEXT("%f"), SpotLightInfo.Attenuation));

}

void USpotLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("Position"));
    if (TempStr)
    {
        SpotLightInfo.Position.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Radius"));
    if (TempStr)
    {
        SpotLightInfo.Radius = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Direction"));
    if (TempStr)
    {
        SpotLightInfo.Direction.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("LightColor"));
    if (TempStr)
    {
        SpotLightInfo.LightColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Intensity"));
    if (TempStr)
    {
        SpotLightInfo.Intensity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Type"));
    if (TempStr)
    {
        SpotLightInfo.Type = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("InnerRad"));
    if (TempStr)
    {
        SpotLightInfo.InnerRad = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("OuterRad"));
    if (TempStr)
    {
        SpotLightInfo.OuterRad = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Attenuation"));
    if (TempStr)
    {
        SpotLightInfo.Attenuation = FString::ToFloat(*TempStr);
    }
}

FVector USpotLightComponent::GetDirection()
{
    FRotator rotator = GetWorldRotation();
    FVector WorldForward = rotator.ToQuaternion().RotateVector(GetForwardVector());
    return WorldForward;
}

const FSpotLightInfo& USpotLightComponent::GetSpotLightInfo() const
{
    return SpotLightInfo;
}

void USpotLightComponent::SetSpotLightInfo(const FSpotLightInfo& InSpotLightInfo)
{
    SpotLightInfo = InSpotLightInfo;
}

float USpotLightComponent::GetRadius() const
{
    return SpotLightInfo.Radius;
}

void USpotLightComponent::SetRadius(float InRadius)
{
    SpotLightInfo.Radius = InRadius;
}

FLinearColor USpotLightComponent::GetLightColor() const
{
    return SpotLightInfo.LightColor;
}

void USpotLightComponent::SetLightColor(const FLinearColor& InColor)
{
    SpotLightInfo.LightColor = InColor;
}



float USpotLightComponent::GetIntensity() const
{
    return SpotLightInfo.Intensity;
}

void USpotLightComponent::SetIntensity(float InIntensity)
{
    SpotLightInfo.Intensity = InIntensity;
}

int USpotLightComponent::GetType() const
{
    return SpotLightInfo.Type;
}

void USpotLightComponent::SetType(int InType)
{
    SpotLightInfo.Type = InType;
}

float USpotLightComponent::GetInnerRad() const
{
    return SpotLightInfo.InnerRad;
}

void USpotLightComponent::SetInnerRad(float InInnerCos)
{
    SpotLightInfo.InnerRad = InInnerCos;
}

float USpotLightComponent::GetOuterRad() const
{
    return SpotLightInfo.OuterRad;
}

void USpotLightComponent::SetOuterRad(float InOuterCos)
{
    SpotLightInfo.OuterRad = InOuterCos;
}

float USpotLightComponent::GetInnerDegree() const
{
    return SpotLightInfo.InnerRad * (180.0f / PI);
}

void USpotLightComponent::SetInnerDegree(float InInnerDegree)
{
    SpotLightInfo.InnerRad = InInnerDegree * (PI / 180.0f);
}

float USpotLightComponent::GetOuterDegree() const
{
    return SpotLightInfo.OuterRad * (180 / PI);
}

void USpotLightComponent::SetOuterDegree(float InOuterDegree)
{
    SpotLightInfo.OuterRad = InOuterDegree * (PI / 180.0f);
}
