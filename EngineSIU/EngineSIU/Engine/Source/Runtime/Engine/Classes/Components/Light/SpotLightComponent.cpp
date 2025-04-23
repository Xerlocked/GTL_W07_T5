#include "SpotLightComponent.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "UObject/Casts.h"

USpotLightComponent::USpotLightComponent()
{
    SpotLightInfo.Position = GetWorldLocation();
    
    SpotLightInfo.AttenuationRadius = 20.0f;
    SpotLightInfo.Direction = USceneComponent::GetForwardVector();
    SpotLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    SpotLightInfo.Intensity = 1000.f;
    SpotLightInfo.Type = ELightType::SPOT_LIGHT;
    SpotLightInfo.InnerRad = 15.f;
    SpotLightInfo.OuterRad = 30.f;
    SpotLightInfo.Falloff = 0.01f;
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
    OutProperties.Add(TEXT("AttenuationRadius"), FString::Printf(TEXT("%f"), SpotLightInfo.AttenuationRadius));
    OutProperties.Add(TEXT("Direction"), FString::Printf(TEXT("%s"), *SpotLightInfo.Direction.ToString()));
    OutProperties.Add(TEXT("LightColor"), FString::Printf(TEXT("%s"), *SpotLightInfo.LightColor.ToString()));
    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), SpotLightInfo.Intensity));
    OutProperties.Add(TEXT("Type"), FString::Printf(TEXT("%d"), SpotLightInfo.Type));
    OutProperties.Add(TEXT("InnerRad"), FString::Printf(TEXT("%f"), SpotLightInfo.InnerRad));
    OutProperties.Add(TEXT("OuterRad"), FString::Printf(TEXT("%f"), SpotLightInfo.OuterRad));
    OutProperties.Add(TEXT("Falloff"), FString::Printf(TEXT("%f"), SpotLightInfo.Falloff));

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
    TempStr = InProperties.Find(TEXT("AttenuationRadius"));
    if (TempStr)
    {
        SpotLightInfo.AttenuationRadius = FString::ToFloat(*TempStr);
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
    TempStr = InProperties.Find(TEXT("Falloff"));
    if (TempStr)
    {
        SpotLightInfo.Falloff = FString::ToFloat(*TempStr);
    }
}

FVector USpotLightComponent::GetDirection()
{
    return GetForwardVector();
}

const FSpotLightInfo& USpotLightComponent::GetSpotLightInfo() const
{
    return SpotLightInfo;
}

void USpotLightComponent::SetSpotLightInfo(const FSpotLightInfo& InSpotLightInfo)
{
    SpotLightInfo = InSpotLightInfo;
}

float USpotLightComponent::GetAttenuationRadius() const
{
    return SpotLightInfo.AttenuationRadius;
}

void USpotLightComponent::SetAttenuationRadius(float InRadius)
{
    SpotLightInfo.AttenuationRadius = InRadius;
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
    return SpotLightInfo.InnerRad;
}

void USpotLightComponent::SetInnerDegree(float InInnerDegree)
{
    if (InInnerDegree > GetOuterDegree())
    {
        SetOuterDegree(InInnerDegree);
    }
    SpotLightInfo.InnerRad = InInnerDegree;
}

float USpotLightComponent::GetOuterDegree() const
{
    return SpotLightInfo.OuterRad;
}

void USpotLightComponent::SetOuterDegree(float InOuterDegree)
{
    if (InOuterDegree < GetInnerDegree())
    {
        SetInnerDegree(InOuterDegree);
    }
    SpotLightInfo.OuterRad = InOuterDegree;
}

float USpotLightComponent::GetFalloff() const
{
    return SpotLightInfo.Falloff;
}

void USpotLightComponent::SetFalloff(float InFalloff)
{
    SpotLightInfo.Falloff = InFalloff;
}
