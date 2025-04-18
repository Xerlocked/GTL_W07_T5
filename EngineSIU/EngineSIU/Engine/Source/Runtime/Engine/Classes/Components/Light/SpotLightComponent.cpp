#include "SpotLightComponent.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
USpotLightComponent::USpotLightComponent()
{
    SpotLightInfo.Position = GetWorldLocation();
    SpotLightInfo.AttenuationRadius = 0.0f;
    SpotLightInfo.Direction = USceneComponent::GetForwardVector();
    SpotLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    SpotLightInfo.Intensity = 1000.0f;
    SpotLightInfo.Type = ELightType::SPOT_LIGHT;
    SpotLightInfo.InnerRad = 0;
    SpotLightInfo.OuterRad = 0;
    SpotLightInfo.Falloff = 0.01f;
}

USpotLightComponent::~USpotLightComponent()
{
}

FVector USpotLightComponent::GetDirection()
{
    return GetWorldMatrix().GetAxis(0).GetSafeNormal();
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
